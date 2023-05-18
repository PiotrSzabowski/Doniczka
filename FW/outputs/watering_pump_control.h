#ifndef WATERING_PUMP_CONTROL_H
#define WATERING_PUMP_CONTROL_H

/** 
 * @brief Controls watering pump task
 * @param pvParamater parameter of task (not used)
*/
void watering_pump_control_task(void *pvParameter);

/** 
 * @brief Sets desired PWM duty cycle on watering pump output pin
 * @param speed duty cycle in %, should range 0 - 100
 * @return OK - when new speed aplied successfully
 *         Error - when given speed is outside the range
 */
esp_err_t watering_pump_set_desired_speed(float speed);

/**
 * @brief Stops the pump
 */
void watering_pump_stop (void);

/**
 * @brief Returns pump speed
 * @return pump speed
 * 
 */
float watering_pump_get_speed (void);
#endif