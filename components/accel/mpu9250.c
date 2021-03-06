#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "mpu9250.h"
#include "mpu9250_spi.h"
#include "esp_log.h"
#include "sdkconfig.h"

const static char* TAG = "mpu9250";

spi_device_handle_t spi;

static const float _accel_lsbs[] =
{
  1.0f / MPU9250_ACCE_SENS_2,
  1.0f / MPU9250_ACCE_SENS_4,
  1.0f / MPU9250_ACCE_SENS_8,
  1.0f / MPU9250_ACCE_SENS_16,
};

static const float _gyro_lsbs[] = 
{
  1.0f / MPU9250_GYRO_SENS_250,
  1.0f / MPU9250_GYRO_SENS_500,
  1.0f / MPU9250_GYRO_SENS_1000,
  1.0f / MPU9250_GYRO_SENS_2000,
};

////////////////////////////////////////////////////////////////////////////////
//
// private utilities
//
////////////////////////////////////////////////////////////////////////////////
static inline void mpu9250_write_reg(uint8_t reg, uint8_t data)
{ 

  if(mpu9250_spi_write(spi, reg, &data, 2) != ESP_OK)
  {
    ESP_LOGE(TAG, "mpu9250_write_reg: failed to mpu9250_spi_write");
  }
  else
  {
    ESP_LOGI(TAG, "mpu9250_write_reg reg:%x, dat:%x",reg, data);
  }
}


static inline uint8_t mpu9250_read_reg(uint8_t reg)
{
  uint8_t ret;

  if(mpu9250_spi_read(spi,reg, &ret, 2) != ESP_OK)
  {
    ESP_LOGE(TAG, "mpu9250_read_reg: failed to mpu9250_spi_read_reg");
  }
  else
  {
    ESP_LOGI(TAG, "mpu9250_read_reg reg:%x, data:%x",reg, ret);
  }
  return ret;
}

static inline void mpu9250_read_data(uint8_t reg, uint8_t* data, uint8_t len)
{ 

  if(mpu9250_spi_read(spi, reg, data, len) != ESP_OK)
  {
    ESP_LOGE(TAG, "mpu9250_read_data: failed to mpu9250_spi_read");
  }
  
} 

uint8_t mpu9250_test_connection()
{
  return mpu9250_read_reg(MPU9250_WHO_AM_I);
}

static inline uint8_t ak8963_read_reg(uint8_t reg)
{
  uint8_t ret;

  if(mpu9250_spi_read(spi,reg, &ret, 1) != ESP_OK)
  {
    ESP_LOGE(TAG, "ak8963_read_reg: failed to mpu9250_spi_read");
  }
  else
  {
    ESP_LOGI(TAG, "ak8963_read_reg reg:%x, data:%x",reg, ret);
  }
  return ret;
}

static inline void ak8963_write_reg(uint8_t reg, uint8_t data)
{ 

  if(mpu9250_spi_write(spi, reg, &data, 2) != ESP_OK)
  {
    ESP_LOGE(TAG, "ak8963_write_reg: failed to mpu9250_spi_write");
  }
  else
  {
    ESP_LOGI(TAG, "ak8963_write_reg reg:%x, data:%x",reg, data);
  }
}

static inline void ak8963_read_data(uint8_t reg, uint8_t* data, uint8_t len)
{ 
  
  if(mpu9250_spi_read(spi, reg, data, len) != ESP_OK)
  {
    ESP_LOGE(TAG, "ak8963_read_data: failed to mpu9250_spi_read");
  }
  else
  {
    ESP_LOGI(TAG, "ak8963_read_data reg:%x, data:%c , len:%d",reg, *data, len);
  }
} 

static void ak8963_init()
{
  uint8_t   v;

  v = ak8963_read_reg(AK8963_WIA);
  ESP_LOGI(TAG, "ak8963 chip ID: %x", v);

  // put ak8963 in mode 2 for 100Hz measurement
  // also set 16 bit output
  v = ((1 << 4) |  0x06);
  ak8963_write_reg(AK8963_CNTL1, v);

  v = ak8963_read_reg(AK8963_CNTL1);
  ESP_LOGI(TAG, "ak8963 control reg: %x", v);
}

static void ak8963_read_all(imu_sensor_data_t* imu)
{
  uint8_t   data[7];

  ak8963_read_data(AK8963_HXL, data, 7);

  imu->mag[0] = (int16_t)(data[1] << 8 | data[0]);
  imu->mag[1] = (int16_t)(data[3] << 8 | data[2]);
  imu->mag[2] = (int16_t)(data[5] << 8 | data[4]);
}

////////////////////////////////////////////////////////////////////////////////
//
// public utilities
//
////////////////////////////////////////////////////////////////////////////////



void mpu9250_init(MPU9250_Accelerometer_t accel_sensitivity, MPU9250_Gyroscope_t gyro_sensitivity)
{ 

  if(mpu9250_spi_set_bus(HSPI_HOST) != ESP_OK){
    ESP_LOGE(TAG, "Set Bus error");
  }

  if(mpu9250_spi_set_addr(&spi) != ESP_OK){
    ESP_LOGE(TAG, "Set Addr error");
  }

  uint8_t wai;
  uint8_t temp;

  // Wakeup MPU6050 
  mpu9250_write_reg(MPU9250_PWR_MGMT_1, 0x00);
  // Disable I2C
  mpu9250_write_reg(MPU9250_USER_CTRL, 0X04);

  /// Config accelerometer 
  temp = mpu9250_read_reg(MPU9250_ACCEL_CONFIG);
  temp = (temp & 0xE7) | (uint8_t)accel_sensitivity << 3;
  mpu9250_write_reg(MPU9250_ACCEL_CONFIG, temp);

  // Config gyroscope 
  temp = mpu9250_read_reg(MPU9250_GYRO_CONFIG);
  temp = (temp & 0xE7) | (uint8_t)gyro_sensitivity << 3;
  mpu9250_write_reg(MPU9250_GYRO_CONFIG, temp);

  wai=mpu9250_test_connection();

  if(wai != 0x71)
  {
      ESP_LOGE(TAG, "NOT FOUND %d",wai);

  }
  else
  {
      ESP_LOGI(TAG,"FOUND'IT %d",wai);
  }
  
  ak8963_init();

}


bool mpu9250_read_gyro_accel(imu_sensor_data_t* imu)
{
  uint8_t data[14];

  // read full raw data
  mpu9250_read_data(MPU9250_ACCEL_XOUT_H, data, 14);
  //mpu9250_spi_read(spi, MPU9250_ACCEL_XOUT_H, data, 14);
  imu->accel[0] = (int16_t)(data[0] << 8 | data[1]);
  imu->accel[1] = (int16_t)(data[2] << 8 | data[3]);
  imu->accel[2] = (int16_t)(data[4] << 8 | data[5]);

  imu->temp     = (data[6] << 8 | data[7]);

  imu->gyro[0]  = (int16_t)(data[8] << 8 | data[9]);
  imu->gyro[1]  = (int16_t)(data[10] << 8 | data[11]);
  imu->gyro[2]  = (int16_t)(data[12] << 8 | data[13]);

  return true;
}


/*
void mpu9250_convert_to_eng_units(imu_sensor_data_t* imu)
{
  imu->accel[0] = imu->accel_raw[0] * mpu9250->accel_lsb;
  imu->accel[1] = imu->accel_raw[1] * mpu9250->accel_lsb;
  imu->accel[2] = imu->accel_raw[2] * mpu9250->accel_lsb;

  imu->temp = (imu->temp_raw / 340333.87f + 21.0f);

  imu->gyro[0]  = imu->gyro_raw[0] * mpu9250->gyro_lsb;
  imu->gyro[1]  = imu->gyro_raw[1] * mpu9250->gyro_lsb;
  imu->gyro[2]  = imu->gyro_raw[2] * mpu9250->gyro_lsb;

  imu->mag[0] = imu->mag_raw[0] * mpu9250->mag_lsb;
  imu->mag[1] = imu->mag_raw[1] * mpu9250->mag_lsb;
  imu->mag[2] = imu->mag_raw[2] * mpu9250->mag_lsb;
}
 */
