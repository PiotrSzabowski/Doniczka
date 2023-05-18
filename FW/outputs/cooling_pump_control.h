#ifndef COOLING_PUMP_CONTROL_H
#define COOLING_PUMP_CONTROL_H

/** 
 * @brief Controls cooling pump task
 * @param pvParamater parameter of task (not used)
*/
void cooling_pump_control_task(void *pvParameter);

/** 
 * @brief Sets PWM duty cycle on cooling pump output pin
 * @param speed duty cycle in %, should range 0 - 100
 * @return OK - when new speed aplied successfully
 *         Error - when given speed is outside the range
 */
esp_err_t cooling_pump_set_speed(float speed);

/**
 * @brief Stops the pump
 */
void cooling_pump_stop (void);

/**
 * @brief Returns pump speed
 * @return pump speed
 * 
 */
float cooling_pump_get_speed (void);
#endif