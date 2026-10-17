
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"

#include "heat_meter.h"
#include "zigbee_meter.h"

static const char *TAG = "MAIN";

// Only for testing of the zigbee meter, remove!
static void kmp_task(void *pvParameters __attribute__((unused)))
{
    heat_meter_t heat_meter;
    heat_meter_init(&heat_meter, CONFIG_KMP_UART_PORT_NUM, CONFIG_KMP_UART_RXD, CONFIG_KMP_UART_TXD);

    while (1)
    {
        struct meter_data_t meter_data = {0};
        if (heat_meter_get_serial(&heat_meter, meter_data.serial, sizeof(meter_data.serial)) == ESP_OK)
        {
            heat_meter_data_t tmp;
            if (heat_meter_read(&heat_meter, &tmp) == ESP_OK && tmp.valid)
            {
                ESP_LOGI(TAG, "Heat meter data:");
                ESP_LOGI(TAG, "  Energy:             %.3f", tmp.energy);
                ESP_LOGI(TAG, "  Volume:             %.3f", tmp.volume);
                ESP_LOGI(TAG, "  Power:              %.3f", tmp.power);
                ESP_LOGI(TAG, "  Flow:               %.3f", tmp.flow);
                ESP_LOGI(TAG, "  Inlet temperature:  %.2f °C", tmp.inlet_temperature);
                ESP_LOGI(TAG, "  Outlet temperature: %.2f °C", tmp.outlet_temperature);

                meter_data.inlet_temperature = tmp.inlet_temperature;
                meter_data.outlet_temperature = tmp.outlet_temperature;
                meter_data.energy = tmp.energy;
                meter_data.volume = tmp.volume;
                meter_data.power = tmp.power;
                meter_data.flow = tmp.flow;
                zigbee_meter_data_handler(&meter_data);
            }
            else
            {
                ESP_LOGE(TAG, "No meter data available...");
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get meter serial...");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    zigbee_meter_init();

    ESP_LOGI(TAG, "Starting KMP task...");
    xTaskCreate(kmp_task, "KMP", 4096, NULL, 5, NULL);
}
