#pragma once

/** 
 * @brief Peltier power control task
 * @param pvParamater parameter of task (not used)
*/
void peltier_power_control_task(void *pvParameter);

/** 
 * @brief Sets PWM duty cycle on peltier output pin
 * @param level duty cycle in %, should range 0 - 100
 * @return OK - when new speed aplied successfully
 *         Error - when given speed is outside the range
 */
esp_err_t peltier_set_power_level(float level);

/**
 * @brief Stops the peltier
 */
void peltier_stop (void);

/**
 * @brief Returns peltier power level
 * @return peltier power level
 * 
 */
float peltier_get_power_level (void);