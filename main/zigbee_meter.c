//////////////////////////////////////////////////////////////////////////
//INCLUDES
//////////////////////////////////////////////////////////////////////////

#include "zigbee_meter.h"

#include "esp_err.h"
#include "esp_zigbee_attribute.h"
#include "esp_zigbee_cluster.h"
#include "esp_zigbee_core.h"
#include "esp_zigbee_type.h"
#include "esp_check.h"
#include "esp_log.h"

#include "freertos/task.h"

#include "nwk/esp_zigbee_nwk.h"
#include "zcl/esp_zigbee_zcl_common.h"
#include "zcl/esp_zigbee_zcl_metering.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////
//DEFINES
//////////////////////////////////////////////////////////////////////////

#define INSTALLCODE_POLICY_ENABLE false
#define ED_AGING_TIMEOUT ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE 3000
#define HA_ESP_HEATING_ENDPOINT 10
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK
#define ZB_INIT_RETRY_PERIOD 2000

#define MANUFACTURER_NAME                                                                                              \
    "\x08"                                                                                                             \
    "Dahlberg"
#define MODEL_IDENTIFIER                                                                                               \
    "\x04"                                                                                                             \
    "MCZB"

///////////////////////////////////////////////////////////////////////////////
//TYPES
///////////////////////////////////////////////////////////////////////////////

struct zigbee_meter_t
{
    bool initialized;
};

//////////////////////////////////////////////////////////////////////////
//VARIABLES
//////////////////////////////////////////////////////////////////////////

static const char *TAG = "ESP_ZB_HEAT";
static struct zigbee_meter_t zigbee_meter;

//////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////

static void meter_task(void *pvParameters);
static void init_zigbee_stack(void);
static esp_err_t action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message);

static void handle_signal_skip_startup(void);
static void handle_signal_device_first_start(esp_err_t status);
static void handle_signal_device_reboot(esp_err_t status);

static void identify_handler(uint8_t identify_on);
static void start_top_level_commissioning_callback(uint8_t mode_mask);
static esp_zb_cluster_list_t *setup_clusters(void);
static void setup_report_configuration(void);
static size_t make_pascal_string(uint8_t *dst, size_t dst_size, const char *src);
static int32_t clamp_to_s24(int32_t value);

//////////////////////////////////////////////////////////////////////////
//FUNCTIONS
//////////////////////////////////////////////////////////////////////////

void zigbee_meter_init(void)
{
    zigbee_meter = (typeof(zigbee_meter)){.initialized = false};

    esp_zb_platform_config_t config = {
        .radio_config = {.radio_mode = ZB_RADIO_MODE_NATIVE},
        .host_config = {.host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE},
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    ESP_LOGI(TAG, "Starting Zigbee task...");
    xTaskCreate(meter_task, "Zigbee_main", 4096, NULL, 4, NULL);
}

void zigbee_meter_data_handler(const struct meter_data_t *data)
{
    ESP_LOGI(TAG,
             "Received meter data: energy=%.2f, volume=%.2f, inlet_temperature=%.2f, outlet_temperature=%.2f, valid=%d",
             data->energy,
             data->volume,
             data->inlet_temperature,
             data->outlet_temperature,
             data->valid);

    if (!zigbee_meter.initialized)
    {
        return;
    }

    uint64_t measured_value = llroundf(data->energy);
    int32_t instantaneous_demand = clamp_to_s24(lroundf(data->power));
    int32_t inlet_temperature = clamp_to_s24(lroundf(data->inlet_temperature * 100.0f));
    int32_t outlet_temperature = clamp_to_s24(lroundf(data->outlet_temperature * 100.0f));
    uint8_t serial[9];
    make_pascal_string(serial, sizeof(serial), data->serial);

    esp_zb_lock_acquire(portMAX_DELAY);
    esp_zb_zcl_set_attribute_val(HA_ESP_HEATING_ENDPOINT,
                                 ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
                                 &measured_value,
                                 false);

    esp_zb_zcl_set_attribute_val(HA_ESP_HEATING_ENDPOINT,
                                 ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID,
                                 &instantaneous_demand,
                                 false);

    esp_zb_zcl_set_attribute_val(HA_ESP_HEATING_ENDPOINT,
                                 ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_METERING_INLET_TEMPERATURE_ID,
                                 &inlet_temperature,
                                 false);

    esp_zb_zcl_set_attribute_val(HA_ESP_HEATING_ENDPOINT,
                                 ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_METERING_OUTLET_TEMPERATURE_ID,
                                 &outlet_temperature,
                                 false);
    esp_zb_zcl_set_attribute_val(HA_ESP_HEATING_ENDPOINT,
                                 ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                 ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                                 ESP_ZB_ZCL_ATTR_METERING_METER_SERIAL_NUMBER_ID,
                                 serial,
                                 false);
    esp_zb_lock_release();
}

//////////////////////////////////////////////////////////////////////////
//LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////

static void meter_task(void *pvParameters __attribute__((unused)))
{
    init_zigbee_stack();

    ESP_ERROR_CHECK(esp_zb_start(false));
    zigbee_meter.initialized = true;
    esp_zb_stack_main_loop();

    ESP_LOGE(TAG, "Zigbee main loop exited unexpectedly");
    vTaskDelete(NULL);
}

static void init_zigbee_stack(void)
{
    /* Initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg = {.esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,
                               .install_code_policy = INSTALLCODE_POLICY_ENABLE,
                               .nwk_cfg.zed_cfg = {.ed_timeout = ED_AGING_TIMEOUT, .keep_alive = ED_KEEP_ALIVE}

    };
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_ep_list_t *endpoint_list = esp_zb_ep_list_create();

    esp_zb_endpoint_config_t metering_endpoint_config = {.endpoint = HA_ESP_HEATING_ENDPOINT,
                                                         .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
                                                         .app_device_id = ESP_ZB_HA_METER_INTERFACE_DEVICE_ID,
                                                         .app_device_version = 0};
    esp_zb_ep_list_add_ep(endpoint_list, setup_clusters(), metering_endpoint_config);
    esp_zb_device_register(endpoint_list);

    esp_zb_core_action_handler_register(action_handler);

    esp_zb_identify_notify_handler_register(HA_ESP_HEATING_ENDPOINT, identify_handler);

    setup_report_configuration();

    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
}

static void handle_signal_skip_startup(void)
{
    ESP_LOGI(TAG, "Initialize Zigbee stack");
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
}

static void handle_signal_device_first_start(esp_err_t status)
{
    ESP_LOGI(TAG, "Zigbee first start");
    if (status == ESP_OK)
    {
        if (esp_zb_bdb_is_factory_new())
        {
            ESP_LOGI(TAG, "Start network steering");
            esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        }
    }
    else
    {
        ESP_LOGW(TAG, "Zigbee first start failed: %s", esp_err_to_name(status));
        esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning_callback,
                               ESP_ZB_BDB_MODE_INITIALIZATION,
                               ZB_INIT_RETRY_PERIOD);
    }
}

static void handle_signal_device_reboot(esp_err_t status)
{
    ESP_LOGI(TAG, "Zigbee start after reboot");
    if (status != ESP_OK)
    {
        ESP_LOGW(TAG, "Zigbee start after reboot failed: %s", esp_err_to_name(status));
        esp_zb_scheduler_alarm((esp_zb_callback_t)start_top_level_commissioning_callback,
                               ESP_ZB_BDB_MODE_INITIALIZATION,
                               ZB_INIT_RETRY_PERIOD);
    }
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    const esp_zb_app_signal_type_t signal_type = *signal_struct->p_app_signal;
    const esp_err_t status = signal_struct->esp_err_status;

    switch (signal_type)
    {
        case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
            handle_signal_skip_startup();
            break;

        case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            handle_signal_device_first_start(status);
            break;

        case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
            handle_signal_device_reboot(status);
            break;

        case ESP_ZB_BDB_SIGNAL_STEERING:
            if (status == ESP_OK)
            {
                esp_zb_ieee_addr_t extended_pan_id;
                esp_zb_get_extended_pan_id(extended_pan_id);
                ESP_LOGI(TAG,
                         "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN "
                         "ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                         extended_pan_id[7],
                         extended_pan_id[6],
                         extended_pan_id[5],
                         extended_pan_id[4],
                         extended_pan_id[3],
                         extended_pan_id[2],
                         extended_pan_id[1],
                         extended_pan_id[0],
                         esp_zb_get_pan_id(),
                         esp_zb_get_current_channel(),
                         esp_zb_get_short_address());
            }
            else
            {
                ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(status));
                esp_zb_scheduler_alarm(
                    (esp_zb_callback_t)start_top_level_commissioning_callback, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
            }
            break;
        default:
            ESP_LOGI(TAG,
                     "ZDO signal: %s (0x%x), status: %s",
                     esp_zb_zdo_signal_to_string(signal_type),
                     signal_type,
                     esp_err_to_name(status));
            break;
    }
}

static esp_err_t action_handler(esp_zb_core_action_callback_id_t callback_id,
                                const void *__attribute__((unused)) message)
{
    ESP_LOGI(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
    return ESP_OK;
}

static void identify_handler(uint8_t identify_on)
{
    ESP_LOGI(TAG, "Identify %s", identify_on ? "on" : "off");
}

static void start_top_level_commissioning_callback(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK,
                        ,
                        TAG,
                        "Failed to start Zigbee bdb commissioning");
}

static esp_zb_cluster_list_t *setup_clusters(void)
{
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();

    // Basic
    esp_zb_basic_cluster_cfg_t basic_cfg = {.zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
                                            .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE};
    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&basic_cfg);
    ESP_ERROR_CHECK(
        esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, MANUFACTURER_NAME));
    ESP_ERROR_CHECK(
        esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, MODEL_IDENTIFIER));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_APPLICATION_VERSION_ID, "0"));
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_basic_cluster(cluster_list, basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    // Identify
    esp_zb_identify_cluster_cfg_t identify_cfg = {.identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE};
    esp_zb_cluster_list_add_identify_cluster(
        cluster_list, esp_zb_identify_cluster_create(&identify_cfg), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    // Metering
    esp_zb_metering_cluster_cfg_t metering_cfg = {.status = ESP_ZB_ZCL_METERING_HCOOL_CHECK_METER,
                                                  .uint_of_measure = ESP_ZB_ZCL_METERING_UNIT_KW_KWH_BINARY,
                                                  .metering_device_type = ESP_ZB_ZCL_METERING_HEAT_METERING};
    esp_zb_attribute_list_t *metering_cluster = esp_zb_metering_cluster_create(&metering_cfg);

    int32_t init_temp = 0;
    ESP_ERROR_CHECK(esp_zb_cluster_add_attr(metering_cluster,
                                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                            ESP_ZB_ZCL_ATTR_METERING_INLET_TEMPERATURE_ID,
                                            ESP_ZB_ZCL_ATTR_TYPE_S24,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &init_temp));
    ESP_ERROR_CHECK(esp_zb_cluster_add_attr(metering_cluster,
                                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                            ESP_ZB_ZCL_ATTR_METERING_OUTLET_TEMPERATURE_ID,
                                            ESP_ZB_ZCL_ATTR_TYPE_S24,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &init_temp));
    int32_t init_demand = 0;
    ESP_ERROR_CHECK(esp_zb_cluster_add_attr(metering_cluster,
                                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                            ESP_ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID,
                                            ESP_ZB_ZCL_ATTR_TYPE_S24,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING,
                                            &init_demand));
    uint8_t init_serial[9] = {0};
    ESP_ERROR_CHECK(esp_zb_cluster_add_attr(metering_cluster,
                                            ESP_ZB_ZCL_CLUSTER_ID_METERING,
                                            ESP_ZB_ZCL_ATTR_METERING_METER_SERIAL_NUMBER_ID,
                                            ESP_ZB_ZCL_ATTR_TYPE_OCTET_STRING,
                                            ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY,
                                            init_serial));

    ESP_ERROR_CHECK(
        esp_zb_cluster_list_add_metering_cluster(cluster_list, metering_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    return cluster_list;
}

static void setup_report_configuration(void)
{
    /* Config the reporting info  */
    esp_zb_zcl_reporting_info_t meter_delivered_reporting_info = {
        .direction = ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
        .ep = HA_ESP_HEATING_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_METERING,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 900,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 900,
        .u.send_info.delta.u48 = {.low = 1, .high = 0},
        .attr_id = ESP_ZB_ZCL_ATTR_METERING_CURRENT_SUMMATION_DELIVERED_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&meter_delivered_reporting_info);

    esp_zb_zcl_reporting_info_t meter_demand_reporting_info = {
        .direction = ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
        .ep = HA_ESP_HEATING_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_METERING,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 1,
        .u.send_info.max_interval = 900,
        .u.send_info.def_min_interval = 1,
        .u.send_info.def_max_interval = 900,
        .u.send_info.delta.u48 = {.low = 1, .high = 0},
        .attr_id = ESP_ZB_ZCL_ATTR_METERING_INSTANTANEOUS_DEMAND_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&meter_demand_reporting_info);

    esp_zb_zcl_reporting_info_t meter_inlet_reporting_info = {
        .direction = ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
        .ep = HA_ESP_HEATING_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_METERING,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 30,
        .u.send_info.max_interval = 900,
        .u.send_info.def_min_interval = 30,
        .u.send_info.def_max_interval = 900,
        .u.send_info.delta.s24 = {.low = 50, .high = 0},
        .attr_id = ESP_ZB_ZCL_ATTR_METERING_INLET_TEMPERATURE_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&meter_inlet_reporting_info);

    esp_zb_zcl_reporting_info_t meter_outlet_reporting_info = {
        .direction = ESP_ZB_ZCL_REPORT_DIRECTION_SEND,
        .ep = HA_ESP_HEATING_ENDPOINT,
        .cluster_id = ESP_ZB_ZCL_CLUSTER_ID_METERING,
        .cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        .dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .u.send_info.min_interval = 30,
        .u.send_info.max_interval = 900,
        .u.send_info.def_min_interval = 30,
        .u.send_info.def_max_interval = 900,
        .u.send_info.delta.s24 = {.low = 50, .high = 0},
        .attr_id = ESP_ZB_ZCL_ATTR_METERING_OUTLET_TEMPERATURE_ID,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };
    esp_zb_zcl_update_reporting_info(&meter_outlet_reporting_info);
}

static size_t make_pascal_string(uint8_t *dst, size_t dst_size, const char *src)
{
    assert(dst);
    assert(src);

    if (dst_size == 0)
    {
        return 0;
    }

    const size_t length = strnlen(src, dst_size - 1);

    dst[0] = (uint8_t)length;
    memcpy(&dst[1], src, length);

    return length;
}

/**
 * @brief Clamps an int32_t value to the range of a signed 24-bit integer (s24).
 *
 * @param value The int32_t value to clamp.
 * @return The clamped int32_t value.
 */
static int32_t clamp_to_s24(int32_t value)
{
    const int32_t min_s24 = -8388608;
    const int32_t max_s24 = 8388607;

    if (value < min_s24)
    {
        return min_s24;
    }
    if (value > max_s24)
    {
        return max_s24;
    }
    return value;
}
