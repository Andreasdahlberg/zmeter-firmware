///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "kmp_client.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "kmp_protocol.h"
#include "kmp_uart.h"

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

#define RX_TIMEOUT_MS 1000

#define KMP_MAX_REGISTERS 8
#define KMP_REGISTER_MAX_RESPONSE_SIZE 9
#define KMP_FRAME_BUFFER_SIZE 128
#define KMP_SI_SIGN 0x80
#define KMP_SI_EXPONENT 0x3F
#define KMP_SI_DECIMAL 0x40

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

typedef struct
{
    uint16_t id;
    const char *name;
} kmp_register_info_t;

///////////////////////////////////////////////////////////////////////////////
//VARIABLES
///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "KMP_CLIENT";

const kmp_register_info_t register_info_table[] = {
    {1, "Energy in"},
    {2, "Energy out"},
    {13, "Energy in hires"},
    {14, "Energy out hires"},
    {60, "Heat Energy (E1)"},
    {61, "Inlet Energy E4"},
    {62, "Outlet Energy E5"},
    {63, "Cooling Energy E3"},
    {64, "Tariff TA2"},
    {65, "Tariff TA3"},
    {66, "Tariff limit 2"},
    {67, "Tariff limit 3"},
    {68, "Volume V1"},
    {69, "Volume V2"},
    {72, "Mass M1"},
    {73, "Mass M2"},
    {74, "Flow V1"},
    {75, "Flow V2"},
    {80, "Current Power"},
    {84, "Pulse input A1"},
    {85, "Pulse input B1"},
    {86, "Temp1"},
    {87, "Temp2"},
    {88, "Temp3"},
    {89, "Tempdiff"},
    {91, "Pressure P1"},
    {92, "Pressure P2"},
    {94, "Heat Energy E2"},
    {95, "Tap water energy E6"},
    {96, "Tap water energy E7"},
    {97, "Temp1xm3 E8"},
    {98, "Target Date"},
    {99, "InfoCode"},
    {104, "Meter number for VB"},
    {110, "Temp2xm3 E9"},
    {112, "Customer number 2"},
    {113, "Infoevent"},
    {114, "Meter number for VA"},
    {122, "Temp4"},
    {123, "MaxFlowDate_Y"},
    {124, "MaxFlow_Y"},
    {125, "MinFlowDate_Y"},
    {126, "MinFlow_Y"},
    {127, "MaxPowerDate_Y"},
    {128, "MaxPower_Y"},
    {129, "MinPowerDate_Y"},
    {130, "MinPower_Y"},
    {138, "MaxFlowDate_M"},
    {139, "MaxFlow_M"},
    {140, "MinFlowDate_M"},
    {141, "MinFlow_M"},
    {142, "MaxPowerDate_M"},
    {143, "MaxPower_M"},
    {144, "MinPowerDate_M"},
    {145, "MinPower_M"},
    {146, "AvgTemp1_Y"},
    {147, "AvgTemp2_Y"},
    {149, "AvgTemp1_M"},
    {150, "AvgTemp2_M"},
    {152, "Program number"},
    {153, "Config number 1"},
    {154, "Software Checksum 1"},
    {168, "Config number 2"},
    {175, "Error hour counter"},
    {178, "Differential energy dE"},
    {179, "Control energy cE"},
    {180, "Differential volume dV"},
    {181, "Control volume cV"},
    {184, "MbusPriAdrMod1"},
    {185, "MbusSekAdrMod1"},
    {218, "MbusPriAdrMod2"},
    {219, "MbusSekAdrMod2"},
    {222, "ConfigChangedEventCount"},
    {224, "Pulse input A2"},
    {225, "Pulse input B2"},
    {228, "Config number 3"},
    {229, "T1_average_autoint"},
    {230, "T2_average_autoint"},
    {234, "l/imp for VA"},
    {235, "l/imp for VB"},
    {239, "Volume V1 hires"},
    {259, "Nominal Q P1 V1"},
    {260, "Nominal Q P2 V2"},
    {266, "E1HighRes"},
    {267, "Cooling energy E3 hires"},
    {346, "Module SW rev"},
    {347, "Customer number"},
    {348, "Date and Time"},
    {355, "COP Year"},
    {362, "Tariff TA4"},
    {364, "Heat energy A1"},
    {365, "Heat energy A2"},
    {366, "T5 limit"},
    {367, "COP Month"},
    {368, "Config number 4"},
    {369, "Info bits"},
    {371, "COP"},
    {372, "Power input B1"},
    {379, "T1 time average day"},
    {380, "T2 time average day"},
    {381, "T1 time average hour"},
    {382, "T2 time average hour"},
    {383, "Flow V1 max year date"},
    {385, "Power max year date"},
    {387, "Flow V1 max month date"},
    {389, "Power max month date"},
    {398, "T1 actual (one decimal)"},
    {399, "T2 actual (one decimal)"},
    {400, "T1-T2 (one decimal)"},
    {404, "Meter Type"},
    {473, "Energy E10"},
    {474, "Energy E11"},
    {477, "T3 time average day"},
    {478, "T3 time average hour"},
    {505, "P1 average day"},
    {506, "P2 average day"},
    {507, "P1 average hour"},
    {508, "P2 average hour"},
    {675, "wM-Bus transmission interval"},
    {1001, "Fabrication No"},
    {1002, "Time"},
    {1003, "Date"},
    {1004, "HourCounter"},
    {1005, "Software edition"},
    {1010, "Customer number 1"},
    {1023, "Power in"},
    {1024, "Power out"},
    {1032, "Operation Mode"},
    {1054, "Voltage L1"},
    {1055, "Voltage L2"},
    {1056, "Voltage L3"},
    {1076, "Current L1"},
    {1077, "Current L2"},
    {1078, "Current L3"},
    {1080, "Power in L1"},
    {1081, "Power in L2"},
    {1082, "Power in L3"},
    {1344, "Power out L1"},
    {1345, "Power out L2"},
    {1346, "Power out L3"},
};

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

static esp_err_t parse_register(const uint8_t *data, size_t data_length, kmp_register_t *reg, size_t *consumed);
static bool get_register_info(kmp_register_id_t register_id, kmp_register_info_t *register_info);
static bool send_request(kmp_client_t *client,
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
    if (!send_request(client, request, sizeof(request), response, sizeof(response), &response_length))
    {
        ESP_LOGE(TAG, "GET_SERIAL request failed");
        return ESP_FAIL;
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
    if (!send_request(client, request, sizeof(request), response, sizeof(response), &response_length))
    {
        ESP_LOGE(TAG, "GET_TYPE request failed");
        return ESP_FAIL;
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

esp_err_t kmp_client_get_registers(kmp_client_t *client,
                                   const kmp_register_id_t *register_ids,
                                   size_t length,
                                   kmp_register_t *registers)
{
    assert(client != NULL);
    assert(registers != NULL);
    assert(register_ids != NULL);

    if (length == 0)
    {
        ESP_LOGE(TAG, "No register IDs specified");
        return ESP_ERR_INVALID_ARG;
    }

    if (length > KMP_MAX_REGISTERS)
    {
        ESP_LOGE(TAG, "Too many register IDs (%zu), max %u", length, KMP_MAX_REGISTERS);
        return ESP_ERR_INVALID_ARG;
    }

    // CID + Length byte + registers
    const size_t request_length = 2 + (length * 2);
    uint8_t request[request_length];
    request[0] = KMP_CID_GET_REGISTER;
    request[1] = (uint8_t)length;

    for (size_t i = 0; i < length; ++i)
    {
        kmp_register_info_t register_info;
        if (!get_register_info(register_ids[i], &register_info))
        {
            ESP_LOGE(TAG, "Invalid register ID (%u)", (uint32_t)register_ids[i]);
            return ESP_ERR_INVALID_ARG;
        }

        const size_t offset = 2 + (i * 2);
        request[offset] = (uint8_t)(register_info.id >> 8);
        request[offset + 1] = (uint8_t)(register_info.id & 0xFF);
    }

    const size_t max_response_length = 2 + (KMP_REGISTER_MAX_RESPONSE_SIZE * length);
    uint8_t response[max_response_length];
    size_t response_length = 0;
    if (!send_request(client, request, sizeof(request), response, sizeof(response), &response_length))
    {
        ESP_LOGE(TAG, "GET_REGISTER request failed");
        return ESP_FAIL;
    }

    // Destination address + CID
    if (response_length < 2)
    {
        ESP_LOGE(TAG, "Invalid GET_REGISTER response length: %zu", response_length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (response[0] != HEAT_METER || response[1] != KMP_CID_GET_REGISTER)
    {
        ESP_LOGE(TAG, "Unexpected GET_REGISTER response: dst=%u, cid=%u", response[0], response[1]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t offset = 2;
    for (size_t i = 0; i < length; ++i)
    {
        kmp_register_info_t register_info;
        if (!get_register_info(register_ids[i], &register_info))
        {
            ESP_LOGE(TAG, "Invalid register ID (%u)", (uint32_t)register_ids[i]);
            return ESP_ERR_INVALID_ARG;
        }

        size_t consumed = 0;
        esp_err_t status = parse_register(&response[offset], response_length - 2, &registers[i], &consumed);
        if (status != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to parse register '%s' (%u) at index %zu", register_info.name, register_info.id, i);
            return status;
        }

        if (registers[i].id != register_info.id)
        {
            ESP_LOGE(TAG, "Unexpected register ID: expected=%u actual=%u", register_info.id, registers[i].id);
            return ESP_ERR_INVALID_RESPONSE;
        }

        offset += consumed;
    }

    if (offset != response_length)
    {
        ESP_LOGE(TAG, "Unexpected data after registers: %zu bytes", response_length - offset);
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

bool kmp_get_register_value_uint32(const kmp_register_t *reg, uint32_t *value)
{
    assert(reg != NULL);
    assert(value != NULL);

    if (reg->length == 0 || reg->length > sizeof(uint32_t))
    {
        ESP_LOGE(TAG, "Invalid value length: %u", reg->length);
        return false;
    }

    uint32_t raw = 0;
    for (size_t i = 0; i < reg->length; ++i)
    {
        raw = (raw << 8) | (uint32_t)reg->value[i];
    }
    *value = raw;

    return true;
}

bool kmp_get_register_value_int32(const kmp_register_t *reg, int32_t *value)
{
    assert(reg != NULL);
    assert(value != NULL);

    uint32_t raw;
    if (!kmp_get_register_value_uint32(reg, &raw))
    {
        return false;
    }

    const bool is_negative = (reg->si_ex & KMP_SI_SIGN) != 0;
    if (is_negative)
    {
        /*
         * -INT32_MAX is representable, but INT32_MIN needs
         * special handling because its magnitude is 2147483648.
         */
        if (raw > (uint32_t)INT32_MAX + 1U)
        {
            ESP_LOGE(TAG, "Register value to large for int32");
            return false;
        }

        if (raw == (uint32_t)INT32_MAX + 1U)
        {
            *value = INT32_MIN;
        }
        else
        {
            *value = -(int32_t)raw;
        }
    }
    else
    {
        if (raw > (uint32_t)INT32_MAX)
        {
            ESP_LOGE(TAG, "Register value to large for int32");
            return false;
        }

        *value = (int32_t)raw;
    }

    return true;
}

bool kmp_get_register_value_float(const kmp_register_t *reg, float *value)
{
    assert(reg != NULL);
    assert(value != NULL);

    uint32_t raw;
    if (!kmp_get_register_value_uint32(reg, &raw))
    {
        return false;
    }

    const bool is_negative = (reg->si_ex & KMP_SI_SIGN) != 0;
    const bool is_decimal = (reg->si_ex & KMP_SI_DECIMAL) != 0;
    const uint8_t exponent = reg->si_ex & KMP_SI_EXPONENT;

    const float factor = pow10f((float)exponent);

    float result = (float)raw;

    if (is_decimal)
    {
        result /= factor;
    }
    else
    {
        result *= factor;
    }

    if (is_negative)
    {
        result = -result;
    }

    if (!isfinite(result))
    {
        return false;
    }

    *value = result;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

static esp_err_t parse_register(const uint8_t *data, size_t data_length, kmp_register_t *reg, size_t *consumed)
{
    assert(data != NULL);
    assert(reg != NULL);
    assert(consumed != NULL);

    *consumed = 0;

    if (data_length < 5)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    reg->id = (uint16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    reg->unit = data[2];
    reg->length = data[3];
    reg->si_ex = data[4];

    if (reg->length > sizeof(reg->value))
    {
        ESP_LOGE(TAG, "Register %u value too large: %u", reg->id, reg->length);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (data_length < 5 + reg->length)
    {
        ESP_LOGE(TAG, "Register %u value truncated: expected=%u available=%zu", reg->id, reg->length, data_length - 5);
        return ESP_ERR_INVALID_RESPONSE;
    }

    memcpy(reg->value, &data[5], reg->length);

    *consumed = 5 + reg->length;

    return ESP_OK;
}

static bool get_register_info(kmp_register_id_t register_id, kmp_register_info_t *register_info)
{
    assert(register_info != NULL);

    if ((uint16_t)register_id < sizeof(register_info_table) / sizeof(register_info_table[0]))
    {
        *register_info = register_info_table[(uint16_t)register_id];
        return true;
    }

    return false;
}


static bool send_request(kmp_client_t *client,
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

    uint8_t frame_buffer[KMP_FRAME_BUFFER_SIZE];
    size_t encoded_length = 0;
    if (kmp_frame_encode(HEAT_METER, request, length, frame_buffer, sizeof(frame_buffer), &encoded_length) !=
        KMP_ENCODE_OK)
    {
        ESP_LOGE(TAG, "Failed to encode KMP frame");
        return false;
    }

    if (kmp_uart_flush(&client->uart) != ESP_OK ||
        kmp_uart_write(&client->uart, frame_buffer, encoded_length) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to write KMP frame");
        return false;
    }

    size_t received_bytes = 0;
    if (kmp_uart_read(&client->uart, frame_buffer, sizeof(frame_buffer), &received_bytes, RX_TIMEOUT_MS) != ESP_OK ||
        received_bytes == 0)
    {
        ESP_LOGE(TAG, "Failed to read KMP response");
        return false;
    }

#if 1
    ESP_LOGD(TAG, "Response (%zu bytes): ", received_bytes);
    for (size_t i = 0; i < received_bytes; i++)
    {
        ESP_LOGD(TAG, "0x%02X ", frame_buffer[i]);
    }
#endif

    size_t consumed = 0;
    kmp_parse_result_t result = kmp_parser_process(&client->parser, frame_buffer, received_bytes, &consumed);

    if (result != KMP_PARSE_FRAME_READY)
    {
        ESP_LOGE(
            TAG, "Failed to parse KMP response: result=%d consumed=%zu received=%zu", result, consumed, received_bytes);
        return false;
    }

    if (client->parser.length > capacity)
    {
        ESP_LOGE(TAG, "KMP response too large");
        return false;
    }

    memcpy(response, client->parser.buffer, client->parser.length);
    *response_length = client->parser.length;

    return true;
}
