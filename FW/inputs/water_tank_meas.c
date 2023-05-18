#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/pcnt.h"
#include "driver/timer.h"
#include "freertos/semphr.h"
#include "../mcu/pinout.h"

#define TIMER_ALARM_VALUE   80000u
#define TIMER_DIVIDER       100u

#define COUNTER_CORRECTION  10u

#define MEASURES_PER_SECOND (1000u/TIMER_DIVIDER)

#define PCNT_INPUT_SIG_IO   ESP_PIN_FREQ_TANK 
#define PCNT_INPUT_CTRL_IO  0 //that pin is IO0 and it can affect on proper counting

#define PCNT_H_LIM_VAL      UINT16_MAX
#define PCNT_L_LIM_VAL      0

#define MIN_WATER_LEVEL     0u
#define MAX_WATER_LEVEL     2000u

static volatile uint32_t last_measured_freq = 0;
static volatile uint16_t last_pcnt_val = 0;

static volatile float tank_level_ml;
static volatile float a_coeff;
static volatile float b_coeff;
static volatile float freq_max;
static volatile float freq_min;
static volatile float tank_max_ml;
static volatile float tank_min_ml;
static volatile uint64_t last_timer_val;

portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;

esp_err_t water_tank_get_level(float* const water_tank_level)
{
    *water_tank_level = tank_level_ml;
    return ESP_OK;
}

uint32_t water_tank_get_frequency(void)
{       
    return last_measured_freq;
}

esp_err_t water_tank_define_max_level(float max_level)
{
    if (MIN_WATER_LEVEL <= max_level && MAX_WATER_LEVEL >= max_level)
    {
        tank_max_ml = max_level;
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}

esp_err_t water_tank_define_min_level(float min_level)
{
    if (MIN_WATER_LEVEL <= min_level && MAX_WATER_LEVEL >= min_level)
    {
        tank_min_ml = min_level;
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }    
}

esp_err_t water_tank_calibrate_max(void)
{
    freq_max = last_measured_freq;
    return ESP_OK;
}

float water_tank_get_max_freq(void)
{
    return freq_max;
}

esp_err_t water_tank_calibrate_min(void)
{
    freq_min = last_measured_freq;
    return ESP_OK;
}

float water_tank_get_min_freq(void)
{
    return freq_min;
}

static void water_tank_init()
{    
    //Timer----------------------------------------------
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_EN
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);
    //End timer------------------------------------------

    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        // Set PCNT input signal and control GPIOs
        .pulse_gpio_num = PCNT_INPUT_SIG_IO,        
        .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_UNIT_0,
        // What to do on the positive / negative edge of pulse input?
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
        .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
        // What to do when control input is low or high?
        .lctrl_mode = PCNT_MODE_REVERSE, // Reverse counting direction if low
        .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
        // Set the maximum and minimum limit values to watch
        .counter_h_lim = PCNT_H_LIM_VAL,
        .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(PCNT_UNIT_0);
}

void water_tank_task(void *pvParameter)
{
    water_tank_init();

    while(1)
    {        
        taskENTER_CRITICAL(&myMutex);
        timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &last_timer_val);
        timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
        pcnt_get_counter_value(PCNT_UNIT_0, &last_pcnt_val);
        pcnt_counter_clear(PCNT_UNIT_0);
        taskEXIT_CRITICAL(&myMutex);

        last_measured_freq = ((TIMER_ALARM_VALUE * last_pcnt_val)/last_timer_val)*COUNTER_CORRECTION;

        //Calculation can be done only when all parameters are set (not zeroed)
        if(0 != freq_min && 0 != freq_max && 0 != tank_min_ml && 0 != tank_max_ml)
        {
            a_coeff = (tank_max_ml - tank_min_ml)/(freq_max - freq_min);
            b_coeff = tank_max_ml - a_coeff * freq_max;            
            tank_level_ml = a_coeff * last_measured_freq + b_coeff;            
        }
        vTaskDelay(100 / portTICK_RATE_MS); //100ms
    }
}