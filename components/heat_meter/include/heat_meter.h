#pragma once

///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "driver/uart.h"
#include "esp_err.h"

#include "kmp_client.h"
#include <stddef.h>

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    kmp_client_t kmp;
    uint32_t serial;
} heat_meter_t;

typedef struct
{
    float energy;
    float volume;
    float power;
    float flow;
    float inlet_temperature;
    float outlet_temperature;
    bool valid;
} heat_meter_data_t;

///////////////////////////////////////////////////////////////////////////////
//FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initializes the heat meter component.
 *
 * @param meter Pointer to the heat meter structure to initialize.
 * @param port The UART port to use for communication.
 * @param rx_pin The RX pin number.
 * @param tx_pin The TX pin number.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t heat_meter_init(heat_meter_t *meter, uart_port_t port, int rx_pin, int tx_pin);

/**
 * @brief Retrieves the unique serial number of the heat meter.
 *
 * @param meter Pointer to the initialized heat meter structure.
 * @param serial Buffer where the serial number will be stored.
 * @param capacity The size of the buffer.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t heat_meter_get_serial(const heat_meter_t *meter, char *serial, size_t capacity);

/**
 * @brief Reads the latest data from the heat meter.
 **
 * @param meter Pointer to the heat meter structure.
 * @param data Pointer to the structure where the read data will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t heat_meter_read(heat_meter_t *meter, heat_meter_data_t *data);
