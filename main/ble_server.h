/**
 * @file ble_client.h
 * 
 * 
 * @author Fernando Zaragoza
 * @brief FIle that includes wrappers around the basic BLE functionality.
 *          This is intented to run as a single client to multiple servers to 
 *          retrieve data from them, by reading the characteristic of each of 
 *          the primary server service.
 * @version 0.1
 * @date 2022-01-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#pragma once

/* * * * * * * * * * * * * * * *
 * * * * * * INCLUDES * * * * *
 * * * * * * * * * * * * * * * */

/* STD */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
/* Non-Volatile Mem Includes */
#include "nvs.h"
#include "nvs_flash.h"

/* ESP32 API */
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "esp_system.h"
/* Vanilla FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

/* SDK */
#include "sdkconfig.h"

/* * * * * * * * * * * * * * * *
 * * * * * * DEFINES * * * * * *
 * * * * * * * * * * * * * * * */

#define DEVICE_NAME            "ESP_GATTS_DEVa"
#define MANUFACTURER_DATA_LEN  17

/* SERVICE A */
#define GATTS_SERVICE_UUID_A    0x00FF   /* Single Server Filter Service UUID for all servers */
#define GATTS_CHAR_UUID_A       0xFF01   /* Single Server Filter Characteristc UUID for all servers */
#define GATTS_DESCR_UUID_A      0x3333
#define GATTS_NUM_HANDLE_A      4
/* SERVICE B */
#define GATTS_SERVICE_UUID_B    0x00EE
#define GATTS_CHAR_UUID_B       0xEE01
#define GATTS_DESCR_UUID_B      0x2222
#define GATTS_NUM_HANDLE_B      4

#define GATTS_CHAR_VAL_LEN_MAX  0x40

#define PREPARE_BUF_MAX_SIZE    1024

#define BLE_SCAN_TIME   1U   // Seconds

/* Server Profile's count. */
#define PROFILE_NUM 2

/* Adv & Scan Rsp flags */
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

/* * * * * * * * * * * * * * * *
 * * * * * FN TYPEDEFS * * * *
 * * * * * * * * * * * * * * * */

/* Callback function when event is triggered; substituting  "esp_gattc_cb_t" typedef*/
typedef void (* esp_gatts_cbk_t)(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t *param, uint8_t idx);

/* * * * * * * * * * * * * * * *
 * * * * * * ENUMS * * * * * * *
 * * * * * * * * * * * * * * * */

typedef enum {
    PROFILE_1_APP_ID = 0,
    PROFILE_2_APP_ID,
    PROFILE_3_APP_ID,
    PROFILE_4_APP_ID,
    PROFILE_5_APP_ID,
} profiles_app_id_t;

/* * * * * * * * * * * * * * * *
 * * * * * * STRUCTS * * * * * *
 * * * * * * * * * * * * * * * */

typedef struct gatts_profile_inst {
    esp_gatts_cbk_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
} gatts_profile_inst_t;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

typedef struct ble_gatt_server
{
    gatts_profile_inst_t  app_profiles[PROFILE_NUM];
    const char           *dev_name;
    uint8_t               adv_config_done;
    esp_attr_value_t      char_val[PROFILE_NUM];
    esp_bt_uuid_t         service_uuid[PROFILE_NUM];     /* Service UUIDs for app profiles. */
    uint16_t              service_handle[PROFILE_NUM];
    esp_bt_uuid_t         charact_uuid[PROFILE_NUM];     /* Same characteristic UUID for all services */
    esp_gatt_char_prop_t  charact_property[PROFILE_NUM];
    prepare_type_env_t    prep_write_env[PROFILE_NUM];
} ble_gatt_server_t;

extern ble_gatt_server_t ble_server;

/***/
void bt_setup(void);

void ble_register_cbs(void);

void ble_setup(void);

void ble_register_app(void);

void ble_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

void ble_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);