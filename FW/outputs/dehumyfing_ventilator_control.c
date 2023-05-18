
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"
#include "dehumyfing_ventilator_control.h"
#include "../mcu/pinout.h"

#define DEHUMYFING_VENTILATOR_PWM_OUTPUT_PIN ESP_PIN_DEHUM_FAN
#define DEHUMYFING_VENTILATOR_PWM_FREQ_HZ 10000u
#define DEHUMYFING_PROCESS_DELAY_MS 100u

static float dehumyfing_ventilator_speed;

void dehumyfing_ventilator_control_task(void *pvParameter)
{
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM0A, DEHUMYFING_VENTILATOR_PWM_OUTPUT_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = DEHUMYFING_VENTILATOR_PWM_FREQ_HZ,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_0, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0f);
    while (1)
    {
        vTaskDelay(DEHUMYFING_PROCESS_DELAY_MS / portTICK_RATE_MS);
    }
}

esp_err_t dehumyfing_ventilator_set_speed(float speed)
{
    esp_err_t ret_val = ESP_FAIL;

    if(0.0f <= speed)
    {
        if(100.0f >= speed)
        {
            mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
            dehumyfing_ventilator_speed = speed;
            ret_val = ESP_OK;
        }
    }
    return ret_val;
}

void dehumyfing_ventilator_stop (void)
{
    mcpwm_set_duty(MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0f);
    dehumyfing_ventilator_speed = 0.0f;
}

float dehumyfing_ventilator_get_speed (void)
{
    return dehumyfing_ventilator_speed;
}