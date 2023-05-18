#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cmd.h"
#include "ascii_art.h"

#define PROMPT_STR CONFIG_IDF_TARGET
#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"
#define RX_BUFFER_SIZE 256u
#define TX_BUFFER_SIZE 0u
#define CMD_HISTORY_MAX_LENGTH 100u
#define MAX_CMDLINE_ARGS 8u
#define MAX_CMDLINE_LENGTH 256u

static const char* TAG = "console_inteface";
static int console_ret;

/*Initialization functions*/
static void initialize_filesystem(void);
static void initialize_nvs(void);
static void initialize_console(void);

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4u,
            .format_if_mount_failed = true,
            .allocation_unit_size = 4096u
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void initialize_console(void)
{
    fflush(stdout);
    fsync(fileno(stdout));

    setvbuf(stdin, NULL, _IONBF, 0u);

    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);

    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    #if SOC_UART_SUPPORT_REF_TICK
            .source_clk = UART_SCLK_REF_TICK,
    #elif SOC_UART_SUPPORT_XTAL_CLK
            .source_clk = UART_SCLK_XTAL,
    #endif
    };

    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, RX_BUFFER_SIZE, TX_BUFFER_SIZE, 0u, NULL, 0u));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    esp_console_config_t console_config = {
        .max_cmdline_args = MAX_CMDLINE_ARGS,
        .max_cmdline_length = MAX_CMDLINE_LENGTH,
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    linenoiseSetMultiLine(1);

    linenoiseSetCompletionCallback(&esp_console_get_completion);

    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    linenoiseHistorySetMaxLen(CMD_HISTORY_MAX_LENGTH);

    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    linenoiseAllowEmpty(true);

    linenoiseHistoryLoad(HISTORY_PATH);
}

void console_interface_task(void *pvParameter)
{
    const char* prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;
    int probe_status = linenoiseProbe();
    char* line;

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    
    initialize_nvs();
    
    initialize_filesystem();

    repl_config.history_save_path = HISTORY_PATH;
    ESP_LOGI(TAG, "Command history enabled");

    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    register_cmd();

    /*Ascii picture of a flower pot*/
    printf(flower_pot);

    printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n"
           "Press Enter or Ctrl+C will terminate the console environment.\n");

    if (probe_status) 
    {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
    }

    while(true) 
    {
        line = linenoise(prompt);
        if (line == NULL)
        {
            break;
        }
        
        if (0u < strlen(line)) 
        {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(HISTORY_PATH);
        }

        /* Try to run the command */
        esp_err_t err = esp_console_run(line, &console_ret);
        if (err == ESP_ERR_NOT_FOUND) 
        {
            ESP_LOGE(TAG, "Unrecognized command\n");
        } 
        else if (err == ESP_ERR_INVALID_ARG) 
        {
            ESP_LOGE(TAG, "Empty command line\n");
        } 
        else if (err == ESP_OK && console_ret!= ESP_OK) 
        {
            ESP_LOGE(TAG, "Command returned non-zero error code: 0x%x (%s)\n", console_ret, esp_err_to_name(console_ret));
        } 
        else if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}