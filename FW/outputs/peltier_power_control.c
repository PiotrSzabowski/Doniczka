
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"
#include "peltier_power_control.h"
#include "../mcu/pinout.h"

#define PELTIER_PWM_OUTPUT_PIN ESP_PIN_PELT
#define PELTIER_PWM_FREQ_HZ 10000u
#define PELTIER_PROCESS_DELAY_MS 100u

static float peltier_power_level;

void peltier_power_control_task(void *pvParameter)
{
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, PELTIER_PWM_OUTPUT_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = PELTIER_PWM_FREQ_HZ,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 0.0f);
    while (1)
    {
        vTaskDelay(PELTIER_PROCESS_DELAY_MS / portTICK_RATE_MS);
    }
}

esp_err_t peltier_set_power_level(float level)
{
    esp_err_t ret_val = ESP_FAIL;

    if(0.0f <= level)
    {
        if(100.0f >= level)
        {
            mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, level);
            peltier_power_level = level;
            ret_val = ESP_OK;
        }
    }
    return ret_val;
}

void peltier_stop (void)
{
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_1, MCPWM_OPR_A, 0.0f);
    peltier_power_level = 0.0f;
}

float peltier_get_power_level (void)
{
    return peltier_power_level;
}