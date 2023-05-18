#include <inttypes.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../third_party/ds18x20.h"
#include <esp_log.h>
#include <esp_err.h>
#include "../mcu/pinout.h"
#include "temperature_sensor.h"

#define MAX_SENSORS 4u

static const gpio_num_t SENSOR_GPIO = ESP_PIN_DS18B20_DATA;
static uint8_t sensor_count = 0u;
static ds18x20_addr_t addrs[MAX_SENSORS];
static float temps[MAX_SENSORS];

static const char *TAG = "temperature_sensor";

void temperature_sensor_task(void *pvParameter)
{
    gpio_set_pull_mode(SENSOR_GPIO, GPIO_PULLUP_ONLY);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


uint8_t temperature_sensor_rescan_devices (void)
{
    sensor_count = 0u;
    esp_err_t res;

    res = ds18x20_scan_devices(SENSOR_GPIO, addrs, MAX_SENSORS, &sensor_count);

    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Sensors scan error %d (%s)", res, esp_err_to_name(res));
    }
    if (0u == sensor_count)
    {
        ESP_LOGW(TAG, "No sensors detected!");
    }
    if (sensor_count > MAX_SENSORS)
    {
        sensor_count = MAX_SENSORS;
    }
    
    return sensor_count;
}

uint8_t temperature_sensor_get_devices_number (void)
{
    return sensor_count;
}

esp_err_t temperature_sensor_get_data (float* temp_C, float* temp_F, uint64_t* const addr, uint8_t  sensor_no)
{
    esp_err_t res;

    if (sensor_no > sensor_count)
    {
        ESP_LOGE(TAG, "Invalid sensor number");
        return ESP_FAIL;
    }
    res = ds18x20_measure_and_read_multi(SENSOR_GPIO, addrs, sensor_count, temps);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Sensors read error %d (%s)", res, esp_err_to_name(res));
        return ESP_FAIL;
    }

    *temp_C = temps[sensor_no];
    *temp_F = (*temp_C) * 1.8 + 32;
    *addr = addrs[sensor_no];

    return ESP_OK;
}