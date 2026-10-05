#pragma once

///////////////////////////////////////////////////////////////////////////////
//INCLUDES
///////////////////////////////////////////////////////////////////////////////

#include "kmp_uart.h"
#include "kmp_protocol.h"
#include <stddef.h>
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
//DEFINES
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Generic macro to get a register value.
 *
 * This macro automatically selects the correct getter function based on the type of
 * the 'value' pointer. It is recommended to use this macro instead of calling
 * the individual getter functions directly.
 *
 * @param reg Pointer to the register to retrieve value from.
 * @param value Pointer to store the retrieved value.
 */
#define kmp_get_register_value(reg, value)                                                                         _Generic((value),                                                                                                          uint32_t *: kmp_get_register_value_uint32,                                                                             int32_t *: kmp_get_register_value_int32,                                                                               float *: kmp_get_register_value_float)((reg), (value))

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    kmp_uart_t uart;
    kmp_parser_t parser;
    uint8_t parse_buffer[32];
} kmp_client_t;

typedef struct
{
    uint8_t meter_type[2];
    uint8_t software_revision[2];
} kmp_meter_type_t;

typedef struct
{
    uint16_t id;
    uint8_t unit;
    uint8_t si_ex;
    uint8_t length;
    uint8_t value[4];
} kmp_register_t;

typedef enum
{
    KMP_REG_ENERGY_IN,
    KMP_REG_ENERGY_OUT,
    KMP_REG_ENERGY_IN_HIRES,
    KMP_REG_ENERGY_OUT_HIRES,
    KMP_REG_HEAT_ENERGY_E1,
    KMP_REG_INLET_ENERGY_E4,
    KMP_REG_OUTLET_ENERGY_E5,
    KMP_REG_COOLING_ENERGY_E3,
    KMP_REG_TARIFF_TA2,
    KMP_REG_TARIFF_TA3,
    KMP_REG_TARIFF_LIMIT_2,
    KMP_REG_TARIFF_LIMIT_3,
    KMP_REG_VOLUME_V1,
    KMP_REG_VOLUME_V2,
    KMP_REG_MASS_M1,
    KMP_REG_MASS_M2,
    KMP_REG_FLOW_V1,
    KMP_REG_FLOW_V2,
    KMP_REG_CURRENT_POWER,
    KMP_REG_PULSE_INPUT_A1,
    KMP_REG_PULSE_INPUT_B1,
    KMP_REG_TEMP1,
    KMP_REG_TEMP2,
    KMP_REG_TEMP3,
    KMP_REG_TEMPDIFF,
    KMP_REG_PRESSURE_P1,
    KMP_REG_PRESSURE_P2,
    KMP_REG_HEAT_ENERGY_E2,
    KMP_REG_TAP_WATER_ENERGY_E6,
    KMP_REG_TAP_WATER_ENERGY_E7,
    KMP_REG_TEMP1XM3_E8,
    KMP_REG_TARGET_DATE,
    KMP_REG_INFOCODE,
    KMP_REG_METER_NUMBER_FOR_VB,
    KMP_REG_TEMP2XM3_E9,
    KMP_REG_CUSTOMER_NUMBER_2,
    KMP_REG_INFOEVENT,
    KMP_REG_METER_NUMBER_FOR_VA,
    KMP_REG_TEMP4,
    KMP_REG_MAXFLOWDATE_Y,
    KMP_REG_MAXFLOW_Y,
    KMP_REG_MINFLOWDATE_Y,
    KMP_REG_MINFLOW_Y,
    KMP_REG_MAXPOWERDATE_Y,
    KMP_REG_MAXPOWER_Y,
    KMP_REG_MINPOWERDATE_Y,
    KMP_REG_MINPOWER_Y,
    KMP_REG_MAXFLOWDATE_M,
    KMP_REG_MAXFLOW_M,
    KMP_REG_MINFLOWDATE_M,
    KMP_REG_MINFLOW_M,
    KMP_REG_MAXPOWERDATE_M,
    KMP_REG_MAXPOWER_M,
    KMP_REG_MINPOWERDATE_M,
    KMP_REG_MINPOWER_M,
    KMP_REG_AVGTEMP1_Y,
    KMP_REG_AVGTEMP2_Y,
    KMP_REG_AVGTEMP1_M,
    KMP_REG_AVGTEMP2_M,
    KMP_REG_PROGRAM_NUMBER,
    KMP_REG_CONFIG_NUMBER_1,
    KMP_REG_SOFTWARE_CHECKSUM_1,
    KMP_REG_CONFIG_NUMBER_2,
    KMP_REG_ERROR_HOUR_COUNTER,
    KMP_REG_DIFFERENTIAL_ENERGY_DE,
    KMP_REG_CONTROL_ENERGY_CE,
    KMP_REG_DIFFERENTIAL_VOLUME_DV,
    KMP_REG_CONTROL_VOLUME_CV,
    KMP_REG_MBUSPRIADRMOD1,
    KMP_REG_MBUSSEKADRMOD1,
    KMP_REG_MBUSPRIADRMOD2,
    KMP_REG_MBUSSEKADRMOD2,
    KMP_REG_CONFIGCHANGEDEVENTCOUNT,
    KMP_REG_PULSE_INPUT_A2,
    KMP_REG_PULSE_INPUT_B2,
    KMP_REG_CONFIG_NUMBER_3,
    KMP_REG_T1_AVERAGE_AUTOINT,
    KMP_REG_T2_AVERAGE_AUTOINT,
    KMP_REG_LIMP_FOR_VA,
    KMP_REG_LIMP_FOR_VB,
    KMP_REG_VOLUME_V1_HIRES,
    KMP_REG_NOMINAL_Q_P1_V1,
    KMP_REG_NOMINAL_Q_P2_V2,
    KMP_REG_E1HIGHRES,
    KMP_REG_COOLING_ENERGY_E3_HIRES,
    KMP_REG_MODULE_SW_REV,
    KMP_REG_CUSTOMER_NUMBER,
    KMP_REG_DATE_AND_TIME,
    KMP_REG_COP_YEAR,
    KMP_REG_TARIFF_TA4,
    KMP_REG_HEAT_ENERGY_A1,
    KMP_REG_HEAT_ENERGY_A2,
    KMP_REG_T5_LIMIT,
    KMP_REG_COP_MONTH,
    KMP_REG_CONFIG_NUMBER_4,
    KMP_REG_INFO_BITS,
    KMP_REG_COP,
    KMP_REG_POWER_INPUT_B1,
    KMP_REG_T1_TIME_AVERAGE_DAY,
    KMP_REG_T2_TIME_AVERAGE_DAY,
    KMP_REG_T1_TIME_AVERAGE_HOUR,
    KMP_REG_T2_TIME_AVERAGE_HOUR,
    KMP_REG_FLOW_V1_MAX_YEAR_DATE,
    KMP_REG_POWER_MAX_YEAR_DATE,
    KMP_REG_FLOW_V1_MAX_MONTH_DATE,
    KMP_REG_POWER_MAX_MONTH_DATE,
    KMP_REG_T1_ACTUAL_ONE_DECIMAL,
    KMP_REG_T2_ACTUAL_ONE_DECIMAL,
    KMP_REG_T1_T2_ONE_DECIMAL,
    KMP_REG_METER_TYPE,
    KMP_REG_ENERGY_E10,
    KMP_REG_ENERGY_E11,
    KMP_REG_T3_TIME_AVERAGE_DAY,
    KMP_REG_T3_TIME_AVERAGE_HOUR,
    KMP_REG_P1_AVERAGE_DAY,
    KMP_REG_P2_AVERAGE_DAY,
    KMP_REG_P1_AVERAGE_HOUR,
    KMP_REG_P2_AVERAGE_HOUR,
    KMP_REG_WM_BUS_TRANSMISSION_INTERVAL,
    KMP_REG_FABRICATION_NO,
    KMP_REG_TIME,
    KMP_REG_DATE,
    KMP_REG_HOURCOUNTER,
    KMP_REG_SOFTWARE_EDITION,
    KMP_REG_CUSTOMER_NUMBER_1,
    KMP_REG_POWER_IN,
    KMP_REG_POWER_OUT,
    KMP_REG_OPERATION_MODE,
    KMP_REG_VOLTAGE_L1,
    KMP_REG_VOLTAGE_L2,
    KMP_REG_VOLTAGE_L3,
    KMP_REG_CURRENT_L1,
    KMP_REG_CURRENT_L2,
    KMP_REG_CURRENT_L3,
    KMP_REG_POWER_IN_L1,
    KMP_REG_POWER_IN_L2,
    KMP_REG_POWER_IN_L3,
    KMP_REG_POWER_OUT_L1,
    KMP_REG_POWER_OUT_L2,
    KMP_REG_POWER_OUT_L3,
} kmp_register_id_t;

///////////////////////////////////////////////////////////////////////////////
//FUNCTION PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initializes the KMP client.
 *
 * @param client Pointer to the kmp_client_t structure to initialize.
 * @param port The UART port to use.
 * @param rx_pin The RX pin number.
 * @param tx_pin The TX pin number.
 */
void kmp_client_init(kmp_client_t *client, uart_port_t port, int rx_pin, int tx_pin);

/**
 * @brief Gets the serial number of the KMP meter.
 *
 * @param client Pointer to the kmp_client_t structure.
 * @param serial Pointer to store the retrieved serial number.
 * @return esp_err_t Error code of the operation.
 */
esp_err_t kmp_client_get_serial(kmp_client_t *client, uint32_t *serial);

/**
 * @brief Gets the meter type and software revision.
 *
 * @param client Pointer to the kmp_client_t structure.
 * @param type Pointer to store the retrieved meter type and revision.
 * @return esp_err_t Error code of the operation.
 */
esp_err_t kmp_client_get_type(kmp_client_t *client, kmp_meter_type_t *type);

/**
 * @brief Gets a list of registers.
 *
 * @param client Pointer to the kmp_client_t structure.
 * @param register_ids Array of register IDs to fetch.
 * @param length Number of register IDs to fetch.
 * @param registers Array of kmp_register_t structures to fill.
 * @return esp_err_t Error code of the operation.
 */
esp_err_t kmp_client_get_registers(kmp_client_t *client,
                                   const kmp_register_id_t *register_ids,
                                   size_t length,
                                   kmp_register_t *registers);

/**
 * @brief Gets the value of a register as a uint32_t.
 *
 * @param reg Pointer to the kmp_register_t structure.
 * @param value Pointer to store the retrieved uint32_t value.
 * @return true if successful, false otherwise.
 */
bool kmp_get_register_value_uint32(const kmp_register_t *reg, uint32_t *value);

/**
 * @brief Gets the value of a register as an int32_t.
 *
 * @param reg Pointer to the kmp_register_t structure.
 * @param value Pointer to store the retrieved int32_t value.
 * @return true if successful, false otherwise.
 */
bool kmp_get_register_value_int32(const kmp_register_t *reg, int32_t *value);

/**
 * @brief Gets the value of a register as a float.
 *
 * @param reg Pointer to the kmp_register_t structure.
 * @param value Pointer to store the retrieved float value.
 * @return true if successful, false otherwise.
 */
bool kmp_get_register_value_float(const kmp_register_t *reg, float *value);
