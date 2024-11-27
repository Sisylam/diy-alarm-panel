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

#include "esp_zigbee_core.h"
#include "light_driver.h"
#include "switch_driver.h"
#include "zcl_utility.h"

/* Zigbee configuration */
#define MAX_CHILDREN                      10                                    /* the max amount of connected devices */
#define INSTALLCODE_POLICY_ENABLE         false                                 /* enable the install code policy for security */
#define HA_ONOFF_SWITCH_ENDPOINT          3                                     /* esp switch device endpoint */
#define HA_COLOR_DIMMABLE_LIGHT_ENDPOINT  2                                     /* esp light device endpoint */
#define HA_IAS_ACE_CLUSTER_ENDPOINT       1                                     /* esp IAS ACE cluster device endpoint */

/* Basic manufacturer information */
#define ESP_MANUFACTURER_NAME "\x05""BOSCH"      /* Customized manufacturer name */
#define ESP_MODEL_IDENTIFIER "\x0B""Alarm Panel" /* Customized model identifier */

#define ZONE_LABEL "\x0C""Panel Button"

#define ESP_ZB_ZR_CONFIG()                                                              \
    {                                                                                   \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,                                       \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,                               \
        .nwk_cfg.zczr_cfg = {                                                           \
            .max_children = MAX_CHILDREN,                                               \
        },                                                                              \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                     \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,   \
    }
