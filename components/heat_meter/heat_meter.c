///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "heat_meter.h"
#include "esp_err.h"
#include "esp_log.h"
#include "kmp_client.h"

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

// 8 for the digits + 1 for NULL
#define METER_SERIAL_BUFFER_SIZE 9

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    kmp_register_id_t id;
    float *destination;
} heat_meter_register_mapping_t;

///////////////////////////////////////////////////////////////////////////////
//VARIABLES
///////////////////////////////////////////////////////////////////////////////

static const char *TAG = "HEAT_METER";

///////////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

static esp_err_t read_registers(heat_meter_t *meter,
                                const heat_meter_register_mapping_t *mapping,
                                kmp_register_t *registers,
                                size_t number_of_registers);
static esp_err_t decode_registers(const heat_meter_register_mapping_t *mapping,
                                  const kmp_register_t *registers,
                                  size_t number_of_registers);

///////////////////////////////////////////////////////////////////////////////
//FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

esp_err_t heat_meter_init(heat_meter_t *meter, uart_port_t port, int rx_pin, int tx_pin)
{
    assert(meter != NULL);
    *meter = (typeof(*meter)){0};

    esp_err_t status = kmp_client_init(&meter->kmp, port, rx_pin, tx_pin);
    if (status != ESP_OK)
    {
        return status;
    }

    status = kmp_client_get_serial(&meter->kmp, &meter->serial);
    if (status != ESP_OK)
    {
        kmp_client_deinit(&meter->kmp);
        *meter = (typeof(*meter)){0};
        ESP_LOGE(TAG, "Failed to read meter serial number");
        return status;
    }

    ESP_LOGI(TAG, "Meter initialized(%" PRIu32 ")", meter->serial);
    return ESP_OK;
}

esp_err_t heat_meter_get_serial(const heat_meter_t *meter, char *serial, size_t capacity)
{
    assert(meter != NULL);
    assert(serial != NULL);

    if (capacity < METER_SERIAL_BUFFER_SIZE)
    {
        ESP_LOGE(TAG, "Insufficient buffer size (%zu)", capacity);
        return ESP_ERR_INVALID_ARG;
    }

    const int result = snprintf(serial, capacity, "%" PRIu32, meter->serial);
    if (result < 0)
    {
        ESP_LOGE(TAG, "Failed to format meter serial");
        return ESP_FAIL;
    }
    if ((size_t)result >= capacity)
    {
        ESP_LOGE(TAG, "Meter serial was truncated (%s)", serial);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t heat_meter_read(heat_meter_t *meter, heat_meter_data_t *data)
{
    assert(meter != NULL);
    assert(data != NULL);

    heat_meter_register_mapping_t mapping[] = {
        {KMP_REG_HEAT_ENERGY_E1, &data->energy},
        {KMP_REG_VOLUME_V1, &data->volume},
        {KMP_REG_CURRENT_POWER, &data->power},
        {KMP_REG_FLOW_V1, &data->flow},
        {KMP_REG_TEMP1, &data->inlet_temperature},
        {KMP_REG_TEMP2, &data->outlet_temperature},
    };

    const size_t number_of_registers = sizeof(mapping) / sizeof(mapping[0]);
    kmp_register_t registers[number_of_registers];

    esp_err_t status = read_registers(meter, mapping, registers, number_of_registers);
    if (status != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read meter data: %s(%i)", esp_err_to_name(status), status);
        *data = (heat_meter_data_t){0};
        return status;
    }

    status = decode_registers(mapping, registers, number_of_registers);
    if (status != ESP_OK)
    {
        *data = (heat_meter_data_t){0};
        return status;
    }

    data->valid = true;
    return ESP_OK;
}

static esp_err_t read_registers(heat_meter_t *meter,
                                const heat_meter_register_mapping_t *mapping,
                                kmp_register_t *registers,
                                size_t number_of_registers)
{
    kmp_register_id_t register_ids[number_of_registers];
    for (size_t i = 0; i < number_of_registers; ++i)
    {
        register_ids[i] = mapping[i].id;
    }

    return kmp_client_get_registers(&meter->kmp, register_ids, number_of_registers, registers);
}

static esp_err_t decode_registers(const heat_meter_register_mapping_t *mapping,
                                  const kmp_register_t *registers,
                                  size_t number_of_registers)
{
    for (size_t i = 0; i < number_of_registers; ++i)
    {
        if (!kmp_get_register_value(&registers[i], mapping[i].destination))
        {
            ESP_LOGE(TAG, "Failed to decode register %u", mapping[i].id);
            return ESP_ERR_INVALID_RESPONSE;
        }
    }

    return ESP_OK;
}
