/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_gap_ble_api.h"
#include "esp_beacon_api.h"

volatile esp_ble_beacon_t robot_adv_beacon = {
        .flags = {0x02, 0x01, 0x06},
        .length = 0x1A,
        .type = 0xFF,
        .uuid = UUID,
        .robot_id = ROBOT_ID,
        .robot_status = 0xffffff,
        .position = 0x02,
        .measured_power = 0xC5
      };

bool esp_ble_check_beacon(esp_ble_beacon_t *rcvd_beacon){
  
  bool check = true;
  uint8_t esp_uuid[UUID_LEN] = UUID;

  if(memcmp(esp_uuid, rcvd_beacon->uuid, sizeof(esp_uuid))!=0)
    check = false;

  return check;
}

esp_err_t esp_ble_config_beacon_data (esp_ble_beacon_t *data_beacon, uint16_t position, uint32_t robot_status){
    
    if ((data_beacon == NULL)){
        return ESP_ERR_INVALID_ARG;
    }

    data_beacon->position = position; 
    data_beacon->robot_status = robot_status;

    return ESP_OK;
}


