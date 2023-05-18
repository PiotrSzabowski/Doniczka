#pragma once

/** 
 * @brief Controls dehumyfing ventilator task
 * @param pvParamater parameter of task (not used)
*/
void dehumyfing_ventilator_control_task(void *pvParameter);

/** 
 * @brief Sets PWM duty cycle on dehumyfing ventilator output pin
 * @param speed duty cycle in %, should range 0 - 100
 * @return OK - when new speed aplied successfully
 *         Error - when given speed is outside the range
 */
esp_err_t dehumyfing_ventilator_set_speed(float speed);

/**
 * @brief Stops the ventilator
 */
void dehumyfing_ventilator_stop (void);

/**
 * @brief Returns ventilator speed
 * @return ventilator speed
 * 
 */
float dehumyfing_ventilator_get_speed (void);
