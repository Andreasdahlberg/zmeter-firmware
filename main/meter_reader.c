///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "heat_meter.h"
#include "zigbee_meter.h"
#include "meter_reader.h"

///////////////////////////////////////////////////////////////////////////////
//VARIABLES
///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "METER_READER";

#define METER_INIT_RETRY_INTERVAL_MS 5000

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

static void dump_heat_meter_data(const heat_meter_data_t *data);

///////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static void meter_reader_task(void *pvParameters __attribute__((unused)))
{
    heat_meter_t heat_meter;
    esp_err_t status;
    while ((status = heat_meter_init(
                &heat_meter, CONFIG_KMP_UART_PORT_NUM, CONFIG_KMP_UART_RXD, CONFIG_KMP_UART_TXD)) != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "Heat meter initialization failed (%s); retrying in %d seconds",
                 esp_err_to_name(status),
                 METER_INIT_RETRY_INTERVAL_MS / 1000);
        vTaskDelay(pdMS_TO_TICKS(METER_INIT_RETRY_INTERVAL_MS));
    }

    zigbee_meter_wait_until_ready();

    while (1)
    {
        struct meter_data_t meter_data = {0};
        if (heat_meter_get_serial(&heat_meter, meter_data.serial, sizeof(meter_data.serial)) == ESP_OK)
        {
            heat_meter_data_t data;
            if (heat_meter_read(&heat_meter, &data) == ESP_OK && data.valid)
            {
                dump_heat_meter_data(&data);

                meter_data.inlet_temperature = data.inlet_temperature;
                meter_data.outlet_temperature = data.outlet_temperature;
                meter_data.energy = data.energy;
                meter_data.volume = data.volume;
                meter_data.power = data.power;
                meter_data.flow = data.flow;
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

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void meter_reader_start(void)
{
    ESP_LOGI(TAG, "Starting meter reader task...");
    xTaskCreate(meter_reader_task, "meter_reader", 4096, NULL, 5, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static void dump_heat_meter_data(const heat_meter_data_t *data)
{
    ESP_LOGD(TAG, "Heat meter data:");
    ESP_LOGD(TAG, "  Energy:             %.3f", data->energy);
    ESP_LOGD(TAG, "  Volume:             %.3f", data->volume);
    ESP_LOGD(TAG, "  Power:              %.3f", data->power);
    ESP_LOGD(TAG, "  Flow:               %.3f", data->flow);
    ESP_LOGD(TAG, "  Inlet temperature:  %.2f °C", data->inlet_temperature);
    ESP_LOGD(TAG, "  Outlet temperature: %.2f °C", data->outlet_temperature);
}
