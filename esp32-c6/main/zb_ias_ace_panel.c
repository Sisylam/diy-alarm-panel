/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Zigbee HA_color_dimmable_light Example
 *
 * This example code is in the Public Domain (or CC0 licensed, at your option.)
 *
 * Unless required by applicable law or agreed to in writing, this
 * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 */

#include "zb_ias_ace_panel.h"

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"

#if !defined CONFIG_ZB_ZCZR
#error Define ZB_ZCZR in idf.py menuconfig to compile light (Router) source code.
#endif

static const char *TAG = "ZB_IAS_ACE_PANEL";

/********************* Define functions **************************/

static switch_func_pair_t button_func_pair[] = {
    {GPIO_INPUT_IO_TOGGLE_SWITCH, SWITCH_ONOFF_TOGGLE_CONTROL}
};

static void zb_buttons_handler(switch_func_pair_t *button_func_pair)
{
    if (button_func_pair->func == SWITCH_ONOFF_TOGGLE_CONTROL) {
        // implemented light switch toggle functionality
        esp_zb_zcl_on_off_cmd_t on_off_cmd;
        //cmd_req.zcl_basic_cmd.src_endpoint = HA_ONOFF_SWITCH_ENDPOINT;
        on_off_cmd.zcl_basic_cmd.src_endpoint = HA_IAS_ACE_CLUSTER_ENDPOINT;
        on_off_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        on_off_cmd.on_off_cmd_id = ESP_ZB_ZCL_CMD_ON_OFF_TOGGLE_ID;
        /*
        esp_zb_zcl_ias_ace_zone_status_changed_cmd_t zone_status_changed_cmd;
        zone_status_changed_cmd.zcl_basic_cmd.src_endpoint = HA_IAS_ACE_CLUSTER_ENDPOINT;
        zone_status_changed_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        zone_status_changed_cmd.payload.zone_id = 0;
        zone_status_changed_cmd.zone_status = ESP_ZB_ZCL_IAS_ZONE_ZONE_STATUS_TEST;
        zone_status_changed_cmd.aud_notification = ESP_ZB_ZCL_IAS_ACE_AUD_NOTIFICATION_DEF_SOUND;
        zone_status_changed_cmd.zone_label = &ZONE_LABEL;
        */
        /*
        esp_zb_zcl_ias_ace_panel_status_changed_cmd_t panel_status_changed_cmd;
        panel_status_changed_cmd.zcl_basic_cmd.src_endpoint = HA_IAS_ACE_CLUSTER_ENDPOINT;
        panel_status_changed_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
        panel_status_changed_cmd.payload.panel_status = ESP_ZB_ZCL_IAS_ACE_PANEL_STATUS_ARMING_NIGHT;
        panel_status_changed_cmd.payload.seconds_remaining = 60;
        panel_status_changed_cmd.payload.aud_notification = ESP_ZB_ZCL_IAS_ACE_AUD_NOTIFICATION_DEF_SOUND;
        panel_status_changed_cmd.payload.alarm_status = ESP_ZB_ZCL_IAS_ACE_ALARM_STATUS_EMERGENCY_PANIC;
        */
        esp_zb_lock_acquire(portMAX_DELAY);
        esp_zb_zcl_on_off_cmd_req(&on_off_cmd);
        //esp_zb_zcl_ias_ace_zone_status_changed_cmd_req(&zone_status_changed_cmd);
        //esp_zb_zcl_ias_ace_panel_status_changed_cmd_req(&panel_status_changed_cmd);
        esp_zb_lock_release();
        ESP_EARLY_LOGI(TAG, "Send 'on_off toggle' command");
    }
}

static esp_err_t deferred_driver_init(void)
{
    light_driver_init(LIGHT_DEFAULT_OFF);
    ESP_RETURN_ON_FALSE(switch_driver_init(button_func_pair, PAIR_SIZE(button_func_pair), zb_buttons_handler), ESP_FAIL, TAG, "Failed to initialize switch driver");
    
    return ESP_OK;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

/**
 * @brief Handles state signal, for example if stack is initialized or network steering is done
 * 
 * @param signal_struct 
 */
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    /* Device rebootet, initializing */
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new())
            {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            }
            else
            {
                ESP_LOGI(TAG, "Device rebooted");
            }
        }
        else
        {
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        }
        else
        {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        if (err_status == ESP_OK)
        {
            if (*(uint8_t *)esp_zb_app_signal_get_params(p_sg_p))
            {
                ESP_LOGI(TAG, "Network(0x%04hx) is open for %d seconds", esp_zb_get_pan_id(), *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p));
            }
            else
            {
                ESP_LOGW(TAG, "Network(0x%04hx) closed, devices joining not allowed.", esp_zb_get_pan_id());
            }
        }
        break;
    case ESP_ZB_NLME_STATUS_INDICATION:
        ESP_LOGI(TAG, "Status param: (0x%x)", *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p));
        break;
    default:
        break;
    }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    bool light_state = 0;
    uint8_t light_level = 0;
    uint16_t light_color_x = 0;
    uint16_t light_color_y = 0;
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);
    if (message->info.dst_endpoint == HA_COLOR_DIMMABLE_LIGHT_ENDPOINT)
    {
        switch (message->info.cluster)
        {
        case ESP_ZB_ZCL_CLUSTER_ID_ON_OFF:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
            {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                light_driver_set_power(light_state);
            }
            else
            {
                ESP_LOGW(TAG, "On/Off cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            break;
        case ESP_ZB_ZCL_CLUSTER_ID_COLOR_CONTROL:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16)
            {
                light_color_x = message->attribute.data.value ? *(uint16_t *)message->attribute.data.value : light_color_x;
                light_color_y = *(uint16_t *)esp_zb_zcl_get_attribute(message->info.dst_endpoint, message->info.cluster,
                                                                      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID)
                                     ->data_p;
                ESP_LOGI(TAG, "Light color x changes to 0x%x", light_color_x);
            }
            else if (message->attribute.id == ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_Y_ID &&
                     message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16)
            {
                light_color_y = message->attribute.data.value ? *(uint16_t *)message->attribute.data.value : light_color_y;
                light_color_x = *(uint16_t *)esp_zb_zcl_get_attribute(message->info.dst_endpoint, message->info.cluster,
                                                                      ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_COLOR_CONTROL_CURRENT_X_ID)
                                     ->data_p;
                ESP_LOGI(TAG, "Light color y changes to 0x%x", light_color_y);
            }
            else
            {
                ESP_LOGW(TAG, "Color control cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            light_driver_set_color_xy(light_color_x, light_color_y);
            break;
        case ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8)
            {
                light_level = message->attribute.data.value ? *(uint8_t *)message->attribute.data.value : light_level;
                light_driver_set_level((uint8_t)light_level);
                ESP_LOGI(TAG, "Light level changes to %d", light_level);
            }
            else
            {
                ESP_LOGW(TAG, "Level Control cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Message data: cluster(0x%x), attribute(0x%x)  ", message->info.cluster, message->attribute.id);
        }
    }
    else if (message->info.dst_endpoint == HA_IAS_ACE_CLUSTER_ENDPOINT)
    {
        switch (message->info.cluster)
        {
        case ESP_ZB_ZCL_CLUSTER_ID_IAS_ZONE:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
            {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                light_driver_set_power(light_state);
            }
            else
            {
                ESP_LOGW(TAG, "IAS Zone cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            break;
        case ESP_ZB_ZCL_CLUSTER_ID_IAS_ACE:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
            {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                light_driver_set_power(light_state);
            }
            else
            {
                ESP_LOGW(TAG, "IAS ACE cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            break;
        case ESP_ZB_ZCL_CLUSTER_ID_ON_OFF:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL)
            {
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                light_driver_set_power(light_state);
            }
            else
            {
                ESP_LOGW(TAG, "On/Off cluster data: attribute(0x%x), type(0x%x)", message->attribute.id, message->attribute.data.type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Message data: cluster(0x%x), attribute(0x%x)  ", message->info.cluster, message->attribute.id);
        }
    }
    return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
    case ESP_ZB_CORE_IDENTIFY_EFFECT_CB_ID:
        ESP_LOGI(TAG, "ESP_ZB_CORE_IDENTIFY_EFFECT_CB_ID : Receive Zigbee action(0x%x) callback", callback_id);
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ESP_LOGI(TAG, "ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID : Receive Zigbee action(0x%x) callback", callback_id);
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_IAS_ACE_PANIC_CB_ID:
        ESP_LOGI(TAG, "ESP_ZB_CORE_IAS_ACE_PANIC_CB_ID : Receive Zigbee action(0x%x) callback", callback_id);
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_IAS_ZONE_ENROLL_RESPONSE_VALUE_CB_ID:
        ESP_LOGI(TAG, "ESP_ZB_CORE_IAS_ZONE_ENROLL_RESPONSE_VALUE_CB_ID : Receive Zigbee action(0x%x) callback", callback_id);
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGW(TAG, "UNKNOWN : Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

/**
  IAS Control and Indicating Equipment (CIE)

   Mandatory Server side:
    - Basic
    - Identify
    - IAS ACE

   Mandatory Client side:
    - IAS WD
    - Identify
    - IAS Zone

  IAS Ancillary Control Equipment (ACE)

   Mandatory Server side:
    - Basic
    - Identify
    - IAS Zone

   Mandatory Client side:
    - IAS ACE
    - Identify
*/
static esp_zb_cluster_list_t *custom_ias_clusters_create()
{
    ESP_LOGI(TAG, "custom_ias_clusters_create");
    esp_zb_ias_zone_cluster_cfg_t ias_zone_cluster_cfg = {
        .zone_state = ESP_ZB_ZCL_IAS_ZONE_ZONESTATE_NOT_ENROLLED,
        .zone_type = ESP_ZB_ZCL_IAS_ZONE_ZONETYPE_STANDARD_CIE,
        .zone_status = 0,
        .ias_cie_addr = ESP_ZB_ZCL_ZONE_IAS_CIE_ADDR_DEFAULT,
        .zone_id = 0,
    };
    esp_zb_identify_cluster_cfg_t identify_cfg;
    identify_cfg.identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    uint8_t groups_name_support_id;
    groups_name_support_id = ESP_ZB_ZCL_GROUPS_NAME_SUPPORT_DEFAULT_VALUE;
    uint8_t scene_name_support_id;
    scene_name_support_id = ESP_ZB_ZCL_SCENES_NAME_SUPPORT_DEFAULT_VALUE;
    esp_zb_on_off_cluster_cfg_t on_off_cfg;
    on_off_cfg.on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE;
    // button cluster
        esp_zb_multistate_value_cluster_cfg_t ias_state_value_cfg = {
            .number_of_states = 11,
            .out_of_service = false,
            .present_value = ESP_ZB_ZCL_IAS_ACE_PANEL_STATUS_DISARMED,
            .status_flags = 0,
        };

    ESP_LOGI(TAG, "esp_zb_zcl_cluster_list_create");
    esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();

    ESP_LOGI(TAG, "esp_zb_basic_cluster_create");
    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_basic_cluster_create(NULL);
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, ESP_MANUFACTURER_NAME));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, ESP_MODEL_IDENTIFIER));
    esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    ESP_LOGI(TAG, "esp_zb_identify_cluster_create (server)");
    esp_zb_attribute_list_t *esp_zb_identify_server_cluster = esp_zb_identify_cluster_create(&identify_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_server_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_identify_cluster_create (client)");
    esp_zb_attribute_list_t *esp_zb_identify_client_cluster = esp_zb_identify_cluster_create(&identify_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_client_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

    ESP_LOGI(TAG, "esp_zb_ias_zone_cluster_create (server)");
    esp_zb_attribute_list_t *ias_zone_server_cluster = esp_zb_ias_zone_cluster_create(&ias_zone_cluster_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ias_zone_cluster(esp_zb_cluster_list, ias_zone_server_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_ias_zone_cluster_create (client)");
    esp_zb_attribute_list_t *ias_zone_client_cluster = esp_zb_ias_zone_cluster_create(&ias_zone_cluster_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ias_zone_cluster(esp_zb_cluster_list, ias_zone_client_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));

    ESP_LOGI(TAG, "esp_zb_ias_ace_cluster_create");
    esp_zb_attribute_list_t *ias_ace_cluster = esp_zb_ias_ace_cluster_create(NULL);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ias_ace_cluster(esp_zb_cluster_list, ias_ace_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_groups_cluster_create");
    esp_zb_attribute_list_t *esp_zb_groups_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_GROUPS);
    esp_zb_groups_cluster_add_attr(esp_zb_groups_cluster, ESP_ZB_ZCL_ATTR_GROUPS_NAME_SUPPORT_ID, &groups_name_support_id);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_groups_cluster(esp_zb_cluster_list, esp_zb_groups_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_scenes_cluster_create");
    esp_zb_attribute_list_t *esp_zb_scenes_cluster = esp_zb_scenes_cluster_create(NULL);
    esp_zb_cluster_update_attr(esp_zb_scenes_cluster, ESP_ZB_ZCL_ATTR_SCENES_NAME_SUPPORT_ID, &scene_name_support_id);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_scenes_cluster(esp_zb_cluster_list, esp_zb_scenes_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_on_off_cluster_create (server)");
    esp_zb_attribute_list_t *esp_zb_on_off_server_cluster = esp_zb_on_off_cluster_create(&on_off_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, esp_zb_on_off_server_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE));

    ESP_LOGI(TAG, "esp_zb_on_off_cluster_create (client)");
    esp_zb_attribute_list_t *esp_zb_on_off_client_cluster = esp_zb_on_off_cluster_create(&on_off_cfg);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, esp_zb_on_off_client_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));
    
    ESP_LOGI(TAG, "esp_zb_ias_wd_cluster_create");
    esp_zb_attribute_list_t *ias_wd_cluster = esp_zb_ias_wd_cluster_create(NULL);
    ESP_ERROR_CHECK(esp_zb_cluster_list_add_ias_wd_cluster(esp_zb_cluster_list, ias_wd_cluster, ESP_ZB_ZCL_CLUSTER_CLIENT_ROLE));
    
    ESP_LOGI(TAG, "esp_zb_multistate_cluster");
    esp_zb_attribute_list_t *esp_zb_multistate_cluster = esp_zb_multistate_value_cluster_create(&ias_state_value_cfg);
    esp_zb_cluster_list_add_multistate_value_cluster(esp_zb_cluster_list, esp_zb_multistate_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    
    return esp_zb_cluster_list;
}

static esp_zb_ep_list_t *custom_ias_ep_create(uint8_t endpoint_id)
{
    ESP_LOGI(TAG, "custom_ias_ep_create");
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = endpoint_id,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_IAS_ANCILLARY_CONTROL_EQUIPMENT_ID,
        .app_device_version = 0};
    esp_zb_ep_list_add_ep(ep_list, custom_ias_clusters_create(), endpoint_config);
    return ep_list;
}

void attr_cb(uint8_t status, uint8_t endpoint, uint16_t cluster_id, uint16_t attr_id, void *new_value)
{
    ESP_LOGI(TAG, "cluster:0x%x, attribute:0x%x changed ", cluster_id, attr_id);
}

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack */
    ESP_LOGI(TAG, "esp_zb_init");
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_ep_list_t *esp_zb_ias_ace_ep = custom_ias_ep_create(HA_IAS_ACE_CLUSTER_ENDPOINT);
    
    /* Register the device */
    ESP_LOGI(TAG, "esp_zb_device_register");
    esp_zb_device_register(esp_zb_ias_ace_ep);

    ESP_LOGI(TAG, "esp_zb_core_action_handler_register");
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
    esp_zb_set_secondary_network_channel_set(ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void app_main(void)
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
