///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "kmp_client.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "kmp_protocol.h"
#include "kmp_uart.h"

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

#define RX_TIMEOUT_MS 2000
#define SERIAL_LENGTH 8

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
    KMP_CID_GET_TYPE = 0x01,
    KMP_CID_GET_SERIAL = 0x02,
    KMP_CID_GET_REGISTER = 0x10,
    KMP_CID_PUT_REGISTER = 0x11,
} kmp_cid_t;

///////////////////////////////////////////////////////////////////////////////
//VARIABLES
///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "KMP_CLIENT";

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

static esp_err_t send_request(kmp_client_t *client,
                              const uint8_t *request,
                              size_t length,
                              uint8_t *response,
                              size_t capacity,
                              size_t *response_length);


///////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
///////////////////////////////////////////////////////////////////////////////


void kmp_client_init(kmp_client_t *client, uart_port_t port, int rx_pin, int tx_pin)
{
    assert(client != NULL);
    *client = (kmp_client_t){0};

    const kmp_uart_config_t uart_config = {.port = port, .rx_buffer_size = 256, .rx_pin = rx_pin, .tx_pin = tx_pin};
    ESP_ERROR_CHECK(kmp_uart_init(&uart_config, &client->uart));
    ESP_LOGI(TAG, "port=%d, rx=%d, tx=%d", port, rx_pin, tx_pin);

    kmp_parser_init(&client->parser, client->parse_buffer, sizeof(client->parse_buffer));
}

esp_err_t kmp_client_get_serial(kmp_client_t *client, uint32_t *serial)
{
    assert(client != NULL);
    assert(serial != NULL);

    const uint8_t request[] = {KMP_CID_GET_SERIAL};
    uint8_t response[32];
    size_t response_length = 0;
    esp_err_t status = send_request(client, request, sizeof(request), response, sizeof(response), &response_length);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "GET_SERIAL request failed: %s", esp_err_to_name(status));
        return status;
    }

    // Destination address + CID + 4 data bytes
    if (response_length != 6)
    {
        ESP_LOGE(TAG, "Invalid GET_SERIAL response length: %zu", response_length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (response[0] != HEAT_METER || response[1] != KMP_CID_GET_SERIAL)
    {
        ESP_LOGE(TAG, "Unexpected GET_SERIAL response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Data starts at index 2 after destination address and CID.
    *serial = ((uint32_t)response[2] << 24) | ((uint32_t)response[3] << 16) | ((uint32_t)response[4] << 8) |
              ((uint32_t)response[5]);
    return ESP_OK;
}

esp_err_t kmp_client_get_type(kmp_client_t *client, kmp_meter_type_t *type)
{
    assert(client != NULL);
    assert(type != NULL);

    const uint8_t request[] = {KMP_CID_GET_TYPE};
    uint8_t response[32];
    size_t response_length = 0;
    esp_err_t status = send_request(client, request, sizeof(request), response, sizeof(response), &response_length);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "GET_TYPE request failed: %s", esp_err_to_name(status));
        return status;
    }

    // Destination address + CID + 4 data bytes
    if (response_length != 6)
    {
        ESP_LOGE(TAG, "Invalid GET_TYPE response length: %zu", response_length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (response[0] != HEAT_METER || response[1] != KMP_CID_GET_TYPE)
    {
        ESP_LOGE(TAG, "Unexpected GET_TYPE response");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Data starts at index 2 after destination address and CID.
    type->meter_type[0] = response[2];
    type->meter_type[1] = response[3];
    type->software_revision[0] = response[4];
    type->software_revision[1] = response[5];

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static esp_err_t send_request(kmp_client_t *client,
                              const uint8_t *request,
                              size_t length,
                              uint8_t *response,
                              size_t capacity,
                              size_t *response_length)
{
    assert(client != NULL);
    assert(request != NULL);
    assert(response != NULL);
    assert(response_length != NULL);

    uint8_t encode_buffer[16];
    size_t encoded_length = 0;
    if (kmp_frame_encode(HEAT_METER, request, length, encode_buffer, sizeof(encode_buffer), &encoded_length) !=
        KMP_ENCODE_OK)
    {
        ESP_LOGE(TAG, "Failed to encode KMP frame");
        return ESP_FAIL;
    }

    if (kmp_uart_flush(&client->uart) != ESP_OK ||
        kmp_uart_write(&client->uart, encode_buffer, encoded_length) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write KMP frame");
        return ESP_FAIL;
    }

    uint8_t rx_buffer[16];
    size_t received_bytes = 0;
    if (kmp_uart_read(&client->uart, rx_buffer, sizeof(rx_buffer), &received_bytes, RX_TIMEOUT_MS) != ESP_OK ||
        received_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read KMP response");
        return ESP_FAIL;
    }

#if 1
    ESP_LOGD(TAG, "Response (%zu bytes): ", received_bytes);
    for (size_t i = 0; i < received_bytes; i++)
    {
        ESP_LOGD(TAG, "0x%02X ", rx_buffer[i]);
    }
#endif

    size_t consumed = 0;
    kmp_parse_result_t result = kmp_parser_process(&client->parser, rx_buffer, received_bytes, &consumed);

    if (result != KMP_PARSE_FRAME_READY)
    {
        ESP_LOGE(TAG, "Failed to parse KMP response");
        return ESP_FAIL;
    }

    if (client->parser.length > capacity)
    {
        ESP_LOGE(TAG, "KMP response too large");
        return ESP_FAIL;
    }

    memcpy(response, client->parser.buffer, client->parser.length);
    *response_length = client->parser.length;

    return ESP_OK;
}
