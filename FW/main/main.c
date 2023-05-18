#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "../outputs/watering_pump_control.h"
#include "../outputs/cooling_pump_control.h"
#include "../cmd/console_interface.h"
#include "../inputs/humidity_sensor.h"
#include "../inputs/temperature_sensor.h"
#include "../outputs/dehumyfing_ventilator_control.h"
#include "../outputs/cooling_ventilator_control.h"
#include "../inputs/water_tank_meas.h"
#include "../outputs/peltier_power_control.h"

void app_main()
{
    xTaskCreate(&watering_pump_control_task, "watering_pump_control_task", 4096, NULL, 5, NULL);
    xTaskCreate(&cooling_pump_control_task, "cooling_pump_control_task", 4096, NULL, 5, NULL);
    xTaskCreate(&console_interface_task, "console_interface_task", 4096, NULL, 10, NULL);
    xTaskCreate(&humidity_sensor_task, "humidity_sensor_task", 4096, NULL, 5, NULL);
    xTaskCreate(&temperature_sensor_task, "temperature_sensor_task", 4096, NULL, 5, NULL);
    xTaskCreate(&cooling_ventilator_control_task, "cooling_ventilator_control_task", 4096, NULL, 5, NULL);
    xTaskCreate(&dehumyfing_ventilator_control_task, "dehumyfing_ventilator_control_task", 4096, NULL, 5, NULL);
    xTaskCreate(&water_tank_task, "water_tank_task", 4096, NULL, 6, NULL);
    xTaskCreate(&peltier_power_control_task, "peltier_power_control_task", 4096, NULL, 5, NULL);
}