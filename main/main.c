
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "meter_reader.h"
#include "zigbee_meter.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    zigbee_meter_start();
    meter_reader_start();
}
