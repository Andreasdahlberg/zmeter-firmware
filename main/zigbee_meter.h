
#pragma once

//////////////////////////////////////////////////////////////////////////
//INCLUDES
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//DEFINES
//////////////////////////////////////////////////////////////////////////

// MOVE from this file!
struct meter_data_t
{
    float energy;
    float volume;
    float power;
    float flow;
    float inlet_temperature;
    float outlet_temperature;
    char serial[9];
    bool valid;
};

/**
 * @brief Initializes the Zigbee meter module.
 *
 * Sets up the necessary resources, peripherals, and internal states
 * required for the Zigbee meter functionality.
 */
void zigbee_meter_init(void);

/**
 * @brief Handler for heat meter data.
 *
 * @param data Pointer to the received meter data structure.
 */
void zigbee_meter_data_handler(const struct meter_data_t *data);
