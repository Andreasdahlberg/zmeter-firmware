
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "esp_log.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "include/kmp_uart.h"

static const char *TAG = "KMP_UART";

esp_err_t kmp_uart_init(const kmp_uart_config_t *config, kmp_uart_t *handle)
{
    assert(config != NULL);
    assert(handle != NULL);

    *handle = (kmp_uart_t){.port = config->port};

    /* Standard KMP UART configuration*/
    uart_config_t uart_config = {
        .baud_rate = 1200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t status = uart_driver_install(handle->port, config->rx_buffer_size, 0, 0, NULL, 0);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(status));
        return status;
    }

    status = uart_param_config(handle->port, &uart_config);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(status));
        uart_driver_delete(handle->port);
        return status;
    }

    status = uart_set_pin(handle->port, config->tx_pin, config->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure UART pins: %s", esp_err_to_name(status));
        uart_driver_delete(handle->port);
        return status;
    }

    ESP_LOGI(TAG, "UART successfully initialized");
    return ESP_OK;
}

void kmp_uart_deinit(kmp_uart_t *handle)
{
    assert(handle != NULL);
    uart_driver_delete(handle->port);
    *handle = (kmp_uart_t){};
}

esp_err_t kmp_uart_write(const kmp_uart_t *handle, const uint8_t *data, size_t length)
{
    assert(handle != NULL);

    int result = uart_write_bytes(handle->port, (const char *)data, length);
    if (result < 0 || (size_t)result != length)
    {
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t
kmp_uart_read(const kmp_uart_t *handle, uint8_t *data, size_t capacity, size_t *bytes_read, uint32_t timeout_ms)
{
    assert(handle != NULL);
    assert(data != NULL);

    int result = uart_read_bytes(handle->port, data, capacity, pdMS_TO_TICKS(timeout_ms));
    if (result < 0)
    {
        return ESP_FAIL;
    }

    *bytes_read = (size_t)result;
    return ESP_OK;
}

esp_err_t kmp_uart_flush(const kmp_uart_t *handle)
{
    assert(handle != NULL);

    return uart_flush(handle->port);
}
