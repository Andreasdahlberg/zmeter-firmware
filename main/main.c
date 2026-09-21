
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_check.h"

#include "kmp_uart.h"
#include "zigbee_meter.h"

static const char *TAG = "MAIN";

// Only for testing of the zigbee meter, remove!
static void kmp_task(void *pvParameters __attribute__((unused)))
{
    kmp_uart_config_t uart_config = {.port = CONFIG_KMP_UART_PORT_NUM,
                                     .rx_buffer_size = 256,
                                     .rx_pin = CONFIG_KMP_UART_RXD,
                                     .tx_pin = CONFIG_KMP_UART_TXD};
    ESP_LOGI(TAG, "port=%d, rx=%d, tx=%d", CONFIG_KMP_UART_PORT_NUM, CONFIG_KMP_UART_RXD, CONFIG_KMP_UART_TXD);

    kmp_uart_t uart_handle;
    kmp_uart_init(&uart_config, &uart_handle);
    while (1)
    {
        const char *data = "Hello world!\n\r";
        kmp_uart_write(&uart_handle, (const uint8_t *)data, strlen(data));
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        struct meter_data_t meter_data = {.inlet_temperature = 74.40f,
                                          .outlet_temperature = 27.53f,
                                          .energy = 32752.0f,
                                          .volume = 512.08f,
                                          .power = 0.9f,
                                          .flow = 17.0f,
                                          .serial = "72529715"};
        zigbee_meter_data_handler(&meter_data);
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    zigbee_meter_init();

    ESP_LOGI(TAG, "Starting KMP task...");
    xTaskCreate(kmp_task, "KMP", 4096, NULL, 5, NULL);
}
