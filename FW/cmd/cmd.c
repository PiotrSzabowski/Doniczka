#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmd.h"
#include "sdkconfig.h"
#include "../outputs/cooling_pump_control.h"
#include "../outputs/watering_pump_control.h"
#include "../outputs/cooling_ventilator_control.h"
#include "../outputs/dehumyfing_ventilator_control.h"
#include "../outputs/peltier_power_control.h"
#include "../inputs/humidity_sensor.h"
#include "../inputs/temperature_sensor.h"
#include "../inputs/water_tank_meas.h"


#define CMD_FUNC_RET_SUCCESS 0
#define CMD_FUNC_RET_FAILURE 1

static struct {
    struct arg_dbl *speed;
    struct arg_end *end;
} cmd_cooling_pump_set_speed_args;

static struct {
    struct arg_dbl *speed;
    struct arg_end *end;
} cmd_watering_pump_set_speed_args;

static struct {
    struct arg_dbl *speed;
    struct arg_end *end;
} cmd_cooling_ventilator_set_speed_args;

static struct {
    struct arg_dbl *speed;
    struct arg_end *end;
} cmd_dehumyfing_ventilator_set_speed_args;

static struct {
    struct arg_dbl *level;
    struct arg_end *end;
} cmd_peltier_set_power_level_args;

static struct {
    struct arg_dbl *level;
    struct arg_end *end;
} cmd_water_tank_define_max_level_args;

static struct {
    struct arg_dbl *level;
    struct arg_end *end;
} cmd_water_tank_define_min_level_args;

static struct {
    struct arg_dbl *level;
    struct arg_end *end;
} cmd_water_tank_calibrate_max_args;

static struct {
    struct arg_dbl *level;
    struct arg_end *end;
} cmd_water_tank_calibrate_min_args;

static const char *TAG = "cmd";

/**
 * @brief Gets version of hardware used 
 * 
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int get_version(void);

/**
 * @brief Restarts the system
 */
static int restart(void);

/**
 * @brief Gets speed of the cooling pump in %
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_cooling_pump_get_speed(void);

/**
 * @brief Sets speed of the cooling pump in %
 * 
 * @param argc arguments count
 * @param argv arguments value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_cooling_pump_set_speed(int argc, char **argv);

/**
 * @brief Sets speed of the watering pump in %
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_watering_pump_get_speed(void);

/**
 * @brief Sets speed of the watering pump in %
 * 
 * @param argc arguments count
 * @param argv arguments value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_watering_pump_set_speed(int argc, char **argv);

/**
 * @brief Gets speed of the cooling ventilator in %
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_cooling_ventilator_get_speed(void);

/**
 * @brief Sets speed of the cooling ventilator in %
 * 
 * @param argc arguments count
 * @param argv arguments value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_cooling_ventilator_set_speed(int argc, char **argv);

/**
 * @brief Gets speed of the dehumyfing ventilator in %
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_dehumyfing_ventilator_get_speed(void);

/**
 * @brief Sets speed of the dehumyfing ventilator in %
 * 
 * @param argc arguments count
 * @param argv arguments value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_dehumyfing_ventilator_set_speed(int argc, char **argv);

/**
 * @brief Reads humidity in %, temperature in Celsius deg and dew point in Celsius deg
 * 
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_humidity_sensor_read(void);

/**
 * @brief Rescans sensors devices, reads temperature in C deg and F deg 
 * 
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_temperature_sensor_read(void);

/**
 * @brief Gets power level of peltier in %
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_peltier_get_power_level(void);

/**
 * @brief Sets power level of peltier in %
 * 
 * @param argc arguments count
 * @param argv arguments value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure
 */
static int cmd_peltier_set_power_level(int argc, char **argv);

/**
 * @brief Gets water tank generator frequency and current water level
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_water_tank_get_info(void);

/**
 * @brief Set up max level of the tank in mililitres 
 * 
 * @param argc argument count 
 * @param argv argument value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure 
 */
static int cmd_water_tank_define_max_level(int argc, char **argv);

/**
 * @brief Set up min level of the tank in mililitres
 * 
 * @param argc argument count
 * @param argv argument value
 * @return CMD_FUNC_RET_SUCCESS for success or CMD_FUNC_RET_FAILURE for failure 
 */
static int cmd_water_tank_define_min_level(int argc, char **argv);

/**
 * @brief Set up curretnt water level as max value of water level
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_water_tank_calibrate_max(void);

/**
 * @brief Set up curretnt water level as min value of water level
 * 
 * @return CMD_FUNC_RET_SUCCESS for success
 */
static int cmd_water_tank_calibrate_min(void);

/*Functions used to register commands above to further use*/
static void register_version(void);
static void register_restart(void);
static void register_cooling_pump_get_speed(void);
static void register_cooling_pump_set_speed(void);
static void register_watering_pump_get_speed(void);
static void register_watering_pump_set_speed(void);
static void register_cooling_ventilator_get_speed(void);
static void register_cooling_ventilator_set_speed(void);
static void register_dehumyfing_ventilator_get_speed(void);
static void register_dehumyfing_ventilator_set_speed(void);
static void register_humidity_sensor_read(void);
static void register_temperature_sensor_read(void);
static void register_peltier_get_power_level(void);
static void register_peltier_set_power_level(void);
static void register_water_tank_get_info(void);
static void register_water_tank_define_max_level(void);
static void register_water_tank_define_min_level(void);
static void register_water_tank_calibrate_max(void);
static void register_water_tank_calibrate_min(void);

void register_cmd(void)
{
    register_version();
    register_restart();
    register_cooling_pump_get_speed();
    register_cooling_pump_set_speed();
    register_watering_pump_get_speed();
    register_watering_pump_set_speed();
    register_cooling_ventilator_get_speed();
    register_cooling_ventilator_set_speed();
    register_dehumyfing_ventilator_get_speed();
    register_dehumyfing_ventilator_set_speed();
    register_humidity_sensor_read();
    register_temperature_sensor_read();
    register_peltier_get_power_level();
    register_peltier_set_power_level();
    register_water_tank_get_info();
    register_water_tank_define_max_level();
    register_water_tank_define_min_level();
    register_water_tank_calibrate_max();
    register_water_tank_calibrate_min();
}

static int get_version(void)
{
    const char *model;
    esp_chip_info_t info;
    uint32_t flash_size;

    esp_chip_info(&info);

    switch(info.model) 
    {
        case CHIP_ESP32:
            model = "ESP32";
            break;
        case CHIP_ESP32S2:
            model = "ESP32-S2";
            break;
        case CHIP_ESP32S3:
            model = "ESP32-S3";
            break;
        case CHIP_ESP32C3:
            model = "ESP32-C3";
            break;
        case CHIP_ESP32H2:
            model = "ESP32-H2";
            break;
        // case CHIP_ESP32C2:
        //     model = "ESP32-C2"; left deliberately, this model does not work
        //     break;
        default:
            model = "Unknown";
            break;
    }

    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) 
    {
        printf("Get flash size failed");
        return CMD_FUNC_RET_FAILURE;
    }
    printf("IDF Version:%s\r\n", esp_get_idf_version());
    printf("Chip info:\r\n");
    printf("\tmodel:%s\r\n", model);
    printf("\tcores:%d\r\n", info.cores);
    printf("\tfeature:%s%s%s%s%d%s\r\n",
           info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
           info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
           info.features & CHIP_FEATURE_BT ? "/BT" : "",
           info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
           flash_size / (1024 * 1024), " MB");
    printf("\trevision number:%d\r\n", info.revision);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_version(void)
{
    const esp_console_cmd_t cmd = {
        .command = "version",
        .help = "Get version of chip and SDK",
        .hint = NULL,
        .func = &get_version,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int restart(void)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

static void register_restart(void)
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Software reset of the chip",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_cooling_pump_get_speed(void)
{
    float speed = cooling_pump_get_speed();
    printf("Cooling pump speed:%.2f\n", speed);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_cooling_pump_get_speed(void)
{
    const esp_console_cmd_t cmd = {
        .command = "cooling_pump_get_speed",
        .help = "Gets speed of cooling pump",
        .hint = NULL,
        .func = &cmd_cooling_pump_get_speed,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_cooling_pump_set_speed(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;
    int nerrors = arg_parse(argc, argv, (void **) &cmd_cooling_pump_set_speed_args);

    if (0u != nerrors) 
    {
        arg_print_errors(stderr, cmd_cooling_pump_set_speed_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot set speed of cooling pump");
        return CMD_FUNC_RET_FAILURE;
    }
    if(0u != cmd_cooling_pump_set_speed_args.speed->count)
    {
        ret = cooling_pump_set_speed((float)(cmd_cooling_pump_set_speed_args.speed->dval[0u]));
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set cooling pump speed");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting cooling pump speed to: %.2f", (float)(cmd_cooling_pump_set_speed_args.speed->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_cooling_pump_set_speed(void)
{
    int num_args = 1;
    cmd_cooling_pump_set_speed_args.speed = arg_dbl0("s", "speed", "<s>", "Speed in 0-100 percent");
    cmd_cooling_pump_set_speed_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "cooling_pump_set_speed",
        .help = "Sets speed of cooling pump",
        .hint = NULL,
        .func = &cmd_cooling_pump_set_speed,
        .argtable = &cmd_cooling_pump_set_speed_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_watering_pump_get_speed(void)
{
    float speed = watering_pump_get_speed();
    printf("watering pump speed:%.2f\n", speed);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_watering_pump_get_speed(void)
{
    const esp_console_cmd_t cmd = {
        .command = "watering_pump_get_speed",
        .help = "Gets speed of watering pump",
        .hint = NULL,
        .func = &cmd_watering_pump_get_speed,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_watering_pump_set_speed(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;
    int nerrors = arg_parse(argc, argv, (void **) &cmd_watering_pump_set_speed_args);

    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_watering_pump_set_speed_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot set speed of watering pump");
        return CMD_FUNC_RET_FAILURE;
    }
    if(0u != cmd_watering_pump_set_speed_args.speed->count)
    {
        ret = watering_pump_set_desired_speed((float)(cmd_watering_pump_set_speed_args.speed->dval[0u]));
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set watering pump speed");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting watering pump speed to: %.2f", (float)(cmd_watering_pump_set_speed_args.speed->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_watering_pump_set_speed(void)
{
    int num_args = 1;
    cmd_watering_pump_set_speed_args.speed = arg_dbl0("s", "speed", "<s>", "Speed in 0-100 percent");
    cmd_watering_pump_set_speed_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "watering_pump_set_speed",
        .help = "Sets speed of watering pump",
        .hint = NULL,
        .func = &cmd_watering_pump_set_speed,
        .argtable = &cmd_watering_pump_set_speed_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_cooling_ventilator_get_speed(void)
{
    float speed = cooling_ventilator_get_speed();
    printf("Cooling ventilator speed:%.2f\n", speed);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_cooling_ventilator_get_speed(void)
{
    const esp_console_cmd_t cmd = {
        .command = "cooling_ventilator_get_speed",
        .help = "Gets speed of cooling ventilator",
        .hint = NULL,
        .func = &cmd_cooling_ventilator_get_speed,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_cooling_ventilator_set_speed(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;
    int nerrors = arg_parse(argc, argv, (void **) &cmd_cooling_ventilator_set_speed_args);

    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_cooling_ventilator_set_speed_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot set speed of cooling ventilator");
        return CMD_FUNC_RET_FAILURE;
    }
    if(0u != cmd_cooling_ventilator_set_speed_args.speed->count)
    {
        ret = cooling_ventilator_set_speed((float)(cmd_cooling_ventilator_set_speed_args.speed->dval[0u]));
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set cooling ventilator speed");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting cooling ventilator speed to: %.2f", (float)(cmd_cooling_ventilator_set_speed_args.speed->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_cooling_ventilator_set_speed(void)
{
    int num_args = 1;
    cmd_cooling_ventilator_set_speed_args.speed = arg_dbl0("s", "speed", "<s>", "Speed in 0-100 percent");
    cmd_cooling_ventilator_set_speed_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "cooling_ventilator_set_speed",
        .help = "Sets speed of cooling ventilator",
        .hint = NULL,
        .func = &cmd_cooling_ventilator_set_speed,
        .argtable = &cmd_cooling_ventilator_set_speed_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_dehumyfing_ventilator_get_speed(void)
{
    float speed = dehumyfing_ventilator_get_speed();
    printf("dehumyfing ventilator speed:%.2f\n", speed);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_dehumyfing_ventilator_get_speed(void)
{
    const esp_console_cmd_t cmd = {
        .command = "dehumyfing_ventilator_get_speed",
        .help = "Gets speed of dehumyfing ventilator",
        .hint = NULL,
        .func = &cmd_dehumyfing_ventilator_get_speed,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_dehumyfing_ventilator_set_speed(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;
    int nerrors = arg_parse(argc, argv, (void **) &cmd_dehumyfing_ventilator_set_speed_args);

    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_dehumyfing_ventilator_set_speed_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot set speed of dehumyfing ventilator");
        return CMD_FUNC_RET_FAILURE;
    }
    if(0u != cmd_dehumyfing_ventilator_set_speed_args.speed->count)
    {
        ret = dehumyfing_ventilator_set_speed((float)(cmd_dehumyfing_ventilator_set_speed_args.speed->dval[0u]));
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set cooling ventilator speed");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting dehumyfing ventilator speed to: %.2f", (float)(cmd_dehumyfing_ventilator_set_speed_args.speed->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_dehumyfing_ventilator_set_speed(void)
{
    int num_args = 1;
    cmd_dehumyfing_ventilator_set_speed_args.speed = arg_dbl0("s", "speed", "<s>", "Speed in 0-100 percent");
    cmd_dehumyfing_ventilator_set_speed_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "dehumyfing_ventilator_set_speed",
        .help = "Sets speed of dehumyfing ventilator",
        .hint = NULL,
        .func = &cmd_dehumyfing_ventilator_set_speed,
        .argtable = &cmd_dehumyfing_ventilator_set_speed_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_humidity_sensor_read(void)
{
    static float temperature = 0u;
    static float humidity = 0u;
    static float dew_point = 0u;

    esp_err_t ret = humidity_sensor_read(&humidity, &temperature, &dew_point);

    if(ESP_OK == ret)
    {
        ESP_LOGI(TAG,"Sensor read success");
        printf("Humidity: %0.1f%%,\tTemperature: %0.1fC,\tDew point: %0.1fC\n\r", humidity, temperature, dew_point);
        return CMD_FUNC_RET_SUCCESS;
    }
    else
    {
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_humidity_sensor_read(void)
{
    const esp_console_cmd_t cmd = {
        .command = "humidity_sensor_read",
        .help = "Reads humidity, temperature and dew point",
        .hint = NULL,
        .func = &cmd_humidity_sensor_read,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_temperature_sensor_read(void)
{
    float temp_C = 0u;
    float temp_F = 0u;
    uint64_t adr = 0u;
    uint8_t sensor_count = temperature_sensor_rescan_devices();
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Found %d sensors\n\r", sensor_count);

    printf("No\tAddress\t\t\tTemp [C]\tTemp [F]\n\r");

    for(int i = 0; i < sensor_count; i++)
    {
        ret = temperature_sensor_get_data(&temp_C, &temp_F, &adr, i);
        if(ret != ESP_OK)
        {
            return CMD_FUNC_RET_FAILURE;
        }
        printf("%d\t%#018"PRIx64"\t%0.2f\t\t%0.2f\n\r", i, adr, temp_C, temp_F);
    }
    
    return CMD_FUNC_RET_SUCCESS;
}

static void register_temperature_sensor_read(void)
{
    const esp_console_cmd_t cmd = {
        .command = "temperature_sensor_read",
        .help = "Rescans temperature sensors, reads temperature in C and F",
        .hint = NULL,
        .func = &cmd_temperature_sensor_read,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_peltier_get_power_level(void)
{
    float level = peltier_get_power_level();
    printf("Peltier power level:%.2f%%\n", level);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_peltier_get_power_level(void)
{
    const esp_console_cmd_t cmd = {
        .command = "peltier_get_power_level",
        .help = "Gets power level of peltier",
        .hint = NULL,
        .func = &cmd_peltier_get_power_level,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_peltier_set_power_level(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;

    int nerrors = arg_parse(argc, argv, (void **) &cmd_peltier_set_power_level_args);
    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_peltier_set_power_level_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot set power level of peltier");
        return CMD_FUNC_RET_FAILURE;
    }
    if(1u == cmd_peltier_set_power_level_args.level->count)
    {
        ret = peltier_set_power_level((float)(cmd_peltier_set_power_level_args.level->dval[0u]));
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to set peltier power level");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting peltier power level to: %.2f%%", (float)(cmd_peltier_set_power_level_args.level->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
}

static void register_peltier_set_power_level(void)
{
    int num_args = 1;
    cmd_peltier_set_power_level_args.level = arg_dbl0("l", "level", "<l>", "Power level in 0-100%%");
    cmd_peltier_set_power_level_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "peltier_set_power_level",
        .help = "Sets power level of peltier",
        .hint = NULL,
        .func = &cmd_peltier_set_power_level,
        .argtable = &cmd_peltier_set_power_level_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_water_tank_get_info(void)
{
    float water_tank_level;

    uint32_t freq = water_tank_get_frequency();
    water_tank_get_level(&water_tank_level);
    printf("Water tank generator frequency: %d Hz\n", freq);
    printf("Water tank level: %f ml\n", water_tank_level);
    return CMD_FUNC_RET_SUCCESS;
}

static void register_water_tank_get_info(void)
{
    const esp_console_cmd_t cmd = {
        .command = "water_tank_get_info",
        .help = "Gets last measured frequency and level of water tank generator",
        .hint = NULL,
        .func = &cmd_water_tank_get_info,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_water_tank_define_max_level(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;    
    int nerrors = arg_parse(argc, argv, (void **) &cmd_water_tank_define_max_level_args);
    
    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_water_tank_define_max_level_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot defined max water tank level");
        return CMD_FUNC_RET_FAILURE;
    }
    if(1u == cmd_water_tank_define_max_level_args.level->count)
    {
        ret = water_tank_define_max_level((float)(cmd_water_tank_define_max_level_args.level->dval[0u]));
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set max water tank level");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting max water tank level: %.2f", (float)(cmd_water_tank_define_max_level_args.level->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
    return CMD_FUNC_RET_SUCCESS;
}

static void register_water_tank_define_max_level()
{
    int num_args = 1;
    cmd_water_tank_define_max_level_args.level = arg_dbl0("l", "level", "<l>", "Level in 0-2000 mililitres");
    cmd_water_tank_define_max_level_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "water_tank_def_max",
        .help = "Gets max value of water tank level",
        .hint = NULL,
        .func = &cmd_water_tank_define_max_level,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_water_tank_define_min_level(int argc, char **argv)
{
    esp_err_t ret = ESP_OK;
    int nerrors = arg_parse(argc, argv, (void **) &cmd_water_tank_define_min_level_args);
    
    if (nerrors != 0) 
    {
        arg_print_errors(stderr, cmd_water_tank_define_min_level_args.end, argv[0u]);
        ESP_LOGE(TAG, "Cannot define min water tank level");
        return CMD_FUNC_RET_FAILURE;
    }
    if(1u == cmd_water_tank_define_min_level_args.level->count)
    {
        ret = water_tank_define_min_level((float)(cmd_water_tank_define_min_level_args.level->dval[0u]));
        
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG,"Failed to set min water tank level");
            return CMD_FUNC_RET_FAILURE;
        }
        else
        {
            ESP_LOGI(TAG, "Setting min water tank level: %.2f", (float)(cmd_water_tank_define_min_level_args.level->dval[0u]));
            return CMD_FUNC_RET_SUCCESS;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Invalid command arguments");
        return CMD_FUNC_RET_FAILURE;
    }
    return CMD_FUNC_RET_SUCCESS;
}

static void register_water_tank_define_min_level()
{
    int num_args = 1;
    cmd_water_tank_define_min_level_args.level = arg_dbl0("l", "level", "<l>", "Level in 0-2000 mililitres");
    cmd_water_tank_define_min_level_args.end = arg_end(num_args);
    const esp_console_cmd_t cmd = {
        .command = "water_tank_def_min",
        .help = "Gets min value of water tank level",
        .hint = NULL,
        .func = &cmd_water_tank_define_min_level,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_water_tank_calibrate_max(void)
{
    water_tank_calibrate_max();
    printf("Water tank calibrate max value: %f Hz\n", water_tank_get_max_freq());
    return CMD_FUNC_RET_SUCCESS;
}

static void register_water_tank_calibrate_max()
{
    const esp_console_cmd_t cmd = {
        .command = "water_tank_calib_max",
        .help = "Calibrate min value of water tank level",
        .hint = NULL,
        .func = &cmd_water_tank_calibrate_max,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int cmd_water_tank_calibrate_min(void)
{
    water_tank_calibrate_min();
    printf("Water tank calibrate min value: %f Hz\n", water_tank_get_min_freq());
    return CMD_FUNC_RET_SUCCESS;
}

static void register_water_tank_calibrate_min()
{
    const esp_console_cmd_t cmd = {
        .command = "water_tank_calib_min",
        .help = "Calibrate max value of water tank level",
        .hint = NULL,
        .func = &cmd_water_tank_calibrate_min,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}