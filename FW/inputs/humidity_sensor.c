#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "../third_party/dht.h"
#include "math.h"
#include "humidity_sensor.h"
#include "../mcu/pinout.h"

#define SENSOR_TYPE DHT_TYPE_AM2301
#define SENSOR_GPIO ESP_PIN_DHT21_DATA

static float temperature = 0u;
static float humidity = 0u;
static float dew_point = 0u;

static const char *TAG = "humidity_sensor";

/**
 * @brief calculate dew point
 * 
 * @param celsius temperature in celsius deg
 * @param humidity humidity in percent
 * @return calculated dew point in celsius deg
 */
static double dew_point_calc(double celsius, double humidity);

void humidity_sensor_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100u));
    }
}

esp_err_t humidity_sensor_read(float* hum, float* temp, float* dew)
{
    esp_err_t ret = ESP_FAIL;

    if (dht_read_float_data(SENSOR_TYPE, SENSOR_GPIO, &humidity, &temperature) == ESP_OK)
    {
        dew_point = (float)dew_point_calc((double)temperature, (double)humidity);

        *temp = temperature;
        *hum = humidity;
        *dew = dew_point;

        ret = ESP_OK;
    }     
    else
    {
        ESP_LOGE(TAG,"Could not read data from sensor\n");
        ret = ESP_FAIL;
    }

    return ret;
}

/*****************************************************
*
* dew point calculation, code downloaded from:
* https://www.best-microcontroller-projects.com/dht22.html
*
******************************************************/
static double dew_point_calc(double celsius, double humidity){

  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}