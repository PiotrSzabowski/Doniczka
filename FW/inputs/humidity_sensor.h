#pragma once

/**
 * @brief humidity sensor sensor task
 * 
 * @param pvParameters parameter of task (not used)
 */
void humidity_sensor_task(void *pvParameters);

/**
 * @brief read data from humidity sensor and print it to the console
 * 
 * @param hum humidity
 * @param temp teperature
 * @param dew dew point 
 * @return ESP_FAIL on fail or ESP_OK on success
 */
esp_err_t humidity_sensor_read(float* hum, float* temp, float* dew);
