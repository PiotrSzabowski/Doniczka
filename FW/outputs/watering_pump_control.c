
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/mcpwm.h"
#include "watering_pump_control.h"
#include "freertos/projdefs.h"
#include "../mcu/pinout.h"

#define WATERING_PUMP_PWM_OUTPUT_PIN ESP_PIN_WATER_PUMP
#define WATERING_PUMP_PWM_FREQ_HZ 100u
#define MAINTENANCE_PERIOD_MS (60u*60u*24u*1000u)
#define WATERING_PUMP_PROCESS_PERIOD_MS 50u
#define MAINTENANCE_PERIOD_CYCLES (MAINTENANCE_PERIOD_MS/WATERING_PUMP_PROCESS_PERIOD_MS)
#define MAINTENANCE_RUN_TIME_MS 500u
#define MAINTENANCE_RUN_TIME_CYCLES (MAINTENANCE_RUN_TIME_MS/WATERING_PUMP_PROCESS_PERIOD_MS)

static float watering_pump_actual_speed;
static float watering_pump_desired_speed;
static bool is_maintenance_run_active = false;
static uint32_t maintenance_run_timer = MAINTENANCE_RUN_TIME_CYCLES;

static void watering_pump_maintenance_run(void);
static void watering_pump_maintenance_process(void);

void watering_pump_control_task(void *pvParameter)
{
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(WATERING_PUMP_PROCESS_PERIOD_MS);

    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, WATERING_PUMP_PWM_OUTPUT_PIN);
    mcpwm_config_t pwm_config = {
        .frequency = WATERING_PUMP_PWM_FREQ_HZ,
        .cmpr_a = 0.0f,
        .cmpr_b = 0.0f,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, 0.0f);
    
    xLastWakeTime = xTaskGetTickCount ();

    while (1)
    {
        xTaskDelayUntil( &xLastWakeTime, xFrequency );

        watering_pump_maintenance_process();
    }
}

static esp_err_t watering_pump_set_speed(float speed)
{
    esp_err_t ret_val = ESP_FAIL;

    if(0.0f <= speed)
    {
        if(100.0f >= speed)
        {
            mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, speed);
            watering_pump_actual_speed = speed;
            ret_val = ESP_OK;
        }
    }
    return ret_val;
}

void watering_pump_stop (void)
{
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, 0.0f);
    watering_pump_actual_speed = 0.0f;
}

float watering_pump_get_speed (void)
{
    return watering_pump_actual_speed;
}

static void watering_pump_maintenance_process(void)
{
    static uint32_t mainetnance_timer = MAINTENANCE_PERIOD_CYCLES;
    
    if(0u != mainetnance_timer)
    {
        if(0.0f == watering_pump_actual_speed)
        {
            mainetnance_timer--;
        }  
        else
        {
            mainetnance_timer = MAINTENANCE_PERIOD_CYCLES;
        }
    }
    else
    {
        is_maintenance_run_active = true;
        mainetnance_timer = MAINTENANCE_PERIOD_CYCLES;
        maintenance_run_timer = MAINTENANCE_RUN_TIME_CYCLES;
    }

    if(true == is_maintenance_run_active)
    {
        watering_pump_maintenance_run();
    }
}

static void watering_pump_maintenance_run(void)
{
    if(0u != maintenance_run_timer)
    {
        maintenance_run_timer--;
        watering_pump_set_speed(100.0f);
    }
    else
    {
        watering_pump_set_speed(watering_pump_desired_speed);
        is_maintenance_run_active = false;
    }
    
}

esp_err_t watering_pump_set_desired_speed(float speed)
{
    esp_err_t ret_val = watering_pump_set_speed(speed);

    if(0.0f != speed)
    {
        if(ESP_OK == ret_val)
        {
            is_maintenance_run_active = false;
        }
    }
    watering_pump_desired_speed = speed;
    
    return ret_val;
}