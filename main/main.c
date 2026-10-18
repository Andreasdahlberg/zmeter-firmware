
#include "esp_err.h"
#include "nvs_flash.h"

#include "meter_reader.h"
#include "zigbee_meter.h"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(zigbee_meter_start());
    meter_reader_start();
}
