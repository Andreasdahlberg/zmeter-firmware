#pragma once

#include "esp_err.h"
#include "driver/uart.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief UART handle structure.
 */
typedef struct kmp_uart
{
    uart_port_t port;
} kmp_uart_t;

/**
 * @brief Configuration structure for UART initialization.
 */
typedef struct
{
    uart_port_t port;
    size_t rx_buffer_size;
    int rx_pin;
    int tx_pin;
} kmp_uart_config_t;

/**
 * @brief Initializes the UART driver with the given configuration.
 *
 * @param config Pointer to the configuration structure.
 * @param handle Pointer to the handle structure to be initialized.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t kmp_uart_init(const kmp_uart_config_t *config, kmp_uart_t *handle);

/**
 * @brief Deinitializes the UART driver.
 *
 * @param handle Pointer to the handle structure to be deinitialized.
 */
void kmp_uart_deinit(kmp_uart_t *handle);

/**
 * @brief Writes data to the UART.
 *
 * @param handle Pointer to the UART handle.
 * @param data Pointer to the data to be written.
 * @param length Length of the data to be written.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t kmp_uart_write(const kmp_uart_t *handle, const uint8_t *data, size_t length);

/**
 * @brief Reads data from the UART.
 *
 * @param handle Pointer to the UART handle.
 * @param data Pointer to the buffer where the read data will be stored.
 * @param capacity Maximum number of bytes to read.
 * @param bytes_read Pointer to a variable where the number of bytes read will be stored.
 * @param timeout_ms Timeout in milliseconds.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t
kmp_uart_read(const kmp_uart_t *handle, uint8_t *data, size_t capacity, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief Flushes the UART transmit buffer.
 *
 * @param handle Pointer to the UART handle.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t kmp_uart_flush(const kmp_uart_t *handle);
