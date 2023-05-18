#pragma once

/**
 * @brief Get water tank level
 * 
 * @param warter_tank_level pointer to water tank level
 * @return ESP_OK for success  
 */
esp_err_t water_tank_get_level(float* const warter_tank_level);

/**
 * @brief Return last measured frequency
 * 
 * @return Last measured frequency
 */
uint32_t water_tank_get_frequency(void);

/**
 * @brief Set up max water tank level in mililitres
 * 
 * @param max_level of water
 * @return ESP_OK for success  
 */
esp_err_t water_tank_define_max_level(float max_level);

/**
 * @brief Set up min water tank level i mililitres
 * 
 * @param min_level of water
 * @return ESP_OK for success  
 */
esp_err_t water_tank_define_min_level(float min_level);

/**
 * @brief Get last measured frequency from water tank pcnt
 * 
 * @return ESP_OK for success 
 */
esp_err_t water_tank_calibrate_max(void);

/**
 * @brief Return frequency of max level of water
 * 
 * @return Last mesured frequency as frequency of max level
 */
float water_tank_get_max_freq(void);

/**
 * @brief Get last measured frequency from water tank pcnt
 * 
 * @return ESP_OK for success
 */
esp_err_t water_tank_calibrate_min(void);

/**
 * @brief Return frequency of min level of water
 * 
 * @return  Last mesured frequency as frequency of min level
 */
float water_tank_get_min_freq(void);

/**
 * @brief Initialize timer and pulse counter
 * 
 */
static void water_tank_init();

/**
 * @brief Water tank task
 * 
 * @param pvParameter Parameter of task (not used)
 */
void water_tank_task(void *pvParameter);