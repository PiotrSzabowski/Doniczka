#pragma once

/**
 * @brief Temperature sensor task
 * 
 * @param pvParameter parameter of task (not used)
 */
void temperature_sensor_task(void *pvParameter);

/**
 * @brief Detects how many devices are connected 
 * 
 * @return uint8_t up-to-date number of detected devices
 */
uint8_t temperature_sensor_rescan_devices (void);

/**
 * @brief Gets the number of devices detected during last scan
 * 
 * @return uint8_t number of devices detected during last scan
 */
uint8_t temperature_sensor_get_devices_number (void);

/**
 * @brief Gets full data of all of the devices 
 * 
 * @param temp_C temperature in Celsius
 * @param temp_F temperature in Fahrenheit
 * @param addr address of the device in hex
 * @param sensor_no numeric index in decimal
 * @return ESP_OK if read was successful, otherwise return ESP_FAIL
 */
esp_err_t temperature_sensor_get_data (float* temp_C, float* temp_F, uint64_t* const addr, uint8_t sensor_no);
