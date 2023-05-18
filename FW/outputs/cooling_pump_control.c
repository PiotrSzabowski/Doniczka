
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"
#include "cooling_pump_control.h"
#include "../mcu/pinout.h"

#define COOLING_PUMP_PWM_OUTPUT_PIN ESP_PIN_COOL_PUMP
#define COOLING_PUMP_PWM_FREQ_HZ 10000u
#define COOLING_PROCESS_DELAY_MS 100u

static float cooling_pump_speed;

void cooling_pump_control_task(void *pvParameter)
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, COOLING_PUMP_PWM_OUTPUT_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = COOLING_PUMP_PWM_FREQ_HZ,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0f);
    while (1)
    {
        vTaskDelay(COOLING_PROCESS_DELAY_MS / portTICK_RATE_MS);
    }
}

esp_err_t cooling_pump_set_speed(float speed)
{
    esp_err_t ret_val = ESP_FAIL;

    if(0.0f <= speed)
    {
        if(100.0f >= speed)
        {
            mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
            cooling_pump_speed = speed;
            ret_val = ESP_OK;
        }
    }
    return ret_val;
}

void cooling_pump_stop (void)
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0f);
    cooling_pump_speed = 0.0f;
}

float cooling_pump_get_speed (void)
{
    return cooling_pump_speed;
}