/**
 * @file ble_server.c
 * 
 * @author Fernando Zaragoza (fezaragozam@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/* * * * * * * * * * * * * * * *
 * * * * * * INCLUDES * * * * *
 * * * * * * * * * * * * * * * */

/* API */
#include "ble_server.h"

/* * * * * * * * * * * * * * * *
 * * * * * * DEFINES * * * * * *
 * * * * * * * * * * * * * * * */

#define BLES_TAG    "BLE_SERVER_DEV"
#define DEBUG       1

///Declare the static function
// static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
// static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_evt_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint8_t idx);

static uint8_t char1_str[] = {0x11,0x22,0x33};
// static esp_gatt_char_prop_t a_property = 0;
// static esp_gatt_char_prop_t b_property = 0;

/* * * * * * * * * * * * * * * *
 * * * * * * STRUCTS * * * * * *
 * * * * * * * * * * * * * * * */

/* API Locals */

// static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd
};
static uint8_t raw_scan_rsp_data[] = {
        0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
        0x45, 0x4d, 0x4f
};
#else

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // First uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    // Second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
// static uint8_t test_manufacturer[MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
// Advertisement Data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp           = false,
    .include_name           = true,
    .include_txpower        = false,
    .min_interval           = 0x0006,   // Slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval           = 0x0010,   // Slave connection max interval, Time = max_interval * 1.25 msec
    .appearance             = 0x00,
    .manufacturer_len       = 0,        // MANUFACTURER_DATA_LEN,
    .p_manufacturer_data    = NULL,     // &test_manufacturer[0],
    .service_data_len       = 0,
    .p_service_data         = NULL,
    .service_uuid_len       = sizeof(adv_service_uuid128),
    .p_service_uuid         = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// Scan Response Data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp           = true,
    .include_name           = true,
    .include_txpower        = true,
    // .min_interval        = 0x0006,
    // .max_interval        = 0x0010,
    .appearance             = 0x00,
    .manufacturer_len       = 0,        // MANUFACTURER_DATA_LEN,
    .p_manufacturer_data    = NULL,     //&test_manufacturer[0],
    .service_data_len       = 0,
    .p_service_data         = NULL,
    .service_uuid_len       = sizeof(adv_service_uuid128),
    .p_service_uuid         = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    // .peer_addr          =
    //.peer_addr_type      =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* API Global */
ble_gatt_server_t ble_server = {
    .app_profiles = {
        /* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
        [PROFILE_1_APP_ID] = {
            .gatts_cb = gatts_profile_evt_handler,
            .gatts_if = ESP_GATT_IF_NONE,           /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        },
        [PROFILE_2_APP_ID] = {
            .gatts_cb = gatts_profile_evt_handler,  /* Not implemented, similar as profile A */
            .gatts_if = ESP_GATT_IF_NONE,           /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        },
    },
    .dev_name        = DEVICE_NAME,
    .adv_config_done = 0,
    .char_val = {
        {
            .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
            .attr_len     = sizeof(char1_str),
            .attr_value   = char1_str,
        },
        {
            .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
            .attr_len     = 0,
            .attr_value   = NULL,
        },
    },
    .service_uuid = {
        {   .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID_A}, },
        {   .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_SERVICE_UUID_B}, },
    },
    .service_handle   = { GATTS_NUM_HANDLE_A,   GATTS_NUM_HANDLE_B },
    .charact_uuid     = {
        {   .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_CHAR_UUID_A}, },
        {   .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = GATTS_CHAR_UUID_B}, },
    },
    .charact_property = { 0, 0 },
    // .prep_write_env /* No initialization for this element */
};

// static prepare_type_env_t a_prepare_write_env;
// static prepare_type_env_t b_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/* * * * * * * * * * * * * * * *
 * * * * FN DEFINITIONS * * * * 
 * * * * * * * * * * * * * * * */

/* API Locals */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_cnf = &ble_server.adv_config_done;
    
    switch (event) 
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            *adv_cnf &= (~adv_config_flag);
            if (*adv_cnf == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            *adv_cnf &= (~scan_rsp_config_flag);
            if (*adv_cnf == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
#else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            *adv_cnf &= (~adv_config_flag);
            if (*adv_cnf == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            *adv_cnf &= (~scan_rsp_config_flag);
            if (*adv_cnf == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
#endif
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            // Advertising start complete event to indicate advertising start successfully or failed
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLES_TAG, "Advertising start failed\n");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(BLES_TAG, "Advertising stop failed\n");
            } else {
                ESP_LOGI(BLES_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(BLES_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                    param->update_conn_params.status,
                    param->update_conn_params.min_int,
                    param->update_conn_params.max_int,
                    param->update_conn_params.conn_int,
                    param->update_conn_params.latency,
                    param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
        if (param->write.is_prep){
            if (prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(BLES_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(BLES_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }else{
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        esp_log_buffer_hex(BLES_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(BLES_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_evt_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint8_t idx)
{
    gatts_profile_inst_t *app_profiles   = ble_server.app_profiles;
    uint8_t              *adv_cnf        = &ble_server.adv_config_done;
    esp_bt_uuid_t        *service_uuid   = ble_server.service_uuid;
    uint16_t             *service_handle = ble_server.service_handle;
    esp_bt_uuid_t        *charact_uuid   = ble_server.charact_uuid;
    esp_gatt_char_prop_t *property       = ble_server.charact_property;
    prepare_type_env_t   *write_env      = ble_server.prep_write_env;

    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(BLES_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
            app_profiles[idx].service_id.is_primary = true;
            app_profiles[idx].service_id.id.inst_id = 0x00;
            app_profiles[idx].service_id.id.uuid.len = ESP_UUID_LEN_16;
            app_profiles[idx].service_id.id.uuid.uuid.uuid16 = service_uuid[idx].uuid.uuid16;

            /* If it's first register event for this device, set device name, and adv */
            if (idx == PROFILE_1_APP_ID)
            {
                esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
                if (set_dev_name_ret){
                    ESP_LOGE(BLES_TAG, "set device name failed, error code = %x", set_dev_name_ret);
                }
#ifdef CONFIG_SET_RAW_ADV_DATA
                esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
                if (raw_adv_ret){
                    ESP_LOGE(BLES_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
                }
                *adv_cnf |= adv_config_flag;
                esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
                if (raw_scan_ret){
                    ESP_LOGE(BLES_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
                }
                *adv_cnf |= scan_rsp_config_flag;
#else
                // Config adv data
                esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
                if (ret){
                    ESP_LOGE(BLES_TAG, "config adv data failed, error code = %x", ret);
                }
                *adv_cnf |= adv_config_flag;
                // Config scan response data
                ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
                if (ret){
                    ESP_LOGE(BLES_TAG, "config scan response data failed, error code = %x", ret);
                }
                *adv_cnf |= scan_rsp_config_flag;
#endif
            }
            esp_ble_gatts_create_service(gatts_if, &app_profiles[idx].service_id, service_handle[idx]);
            break;

        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(BLES_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 4;
            rsp.attr_value.value[0] = 0xde;
            rsp.attr_value.value[1] = 0xed;
            rsp.attr_value.value[2] = 0xbe;
            rsp.attr_value.value[3] = 0xef;
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                        ESP_GATT_OK, &rsp);
            break;
        }

    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(BLES_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(BLES_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(BLES_TAG, param->write.value, param->write.len);
            if (app_profiles[idx].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = (param->write.value[1] << 8) | param->write.value[0];
                if (descr_value == 0x0001) {
                    if (property[idx] & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                        ESP_LOGI(BLES_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, app_profiles[idx].char_handle,
                                                sizeof(notify_data), notify_data, false);
                    }
                } 
                else if (descr_value == 0x0002) {
                    if (property[idx] & ESP_GATT_CHAR_PROP_BIT_INDICATE) {
                        ESP_LOGI(BLES_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, app_profiles[idx].char_handle,
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000) {
                    ESP_LOGI(BLES_TAG, "notify/indicate disable ");
                }
                else {
                    ESP_LOGE(BLES_TAG, "unknown descr value");
                    esp_log_buffer_hex(BLES_TAG, param->write.value, param->write.len);
                }
            }
        }
        example_write_event_env(gatts_if, &write_env[idx], param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(BLES_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&write_env[idx], param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(BLES_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(BLES_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        app_profiles[idx].service_handle = param->create.service_handle;
        app_profiles[idx].char_uuid.len = ESP_UUID_LEN_16;
        app_profiles[idx].char_uuid.uuid.uuid16 = charact_uuid[idx].uuid.uuid16;

        esp_ble_gatts_start_service(app_profiles[idx].service_handle);
        property[idx] = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret = esp_ble_gatts_add_char( app_profiles[idx].service_handle, &app_profiles[idx].char_uuid,
                                                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                         property[idx],
                                                         &ble_server.char_val[idx], NULL);
        if (add_char_ret){
            ESP_LOGE(BLES_TAG, "add char failed, error code =%x",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(BLES_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        app_profiles[idx].char_handle = param->add_char.attr_handle;
        app_profiles[idx].descr_uuid.len = ESP_UUID_LEN_16;
        app_profiles[idx].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL){
            ESP_LOGE(BLES_TAG, "ILLEGAL HANDLE");
        }

        ESP_LOGI(BLES_TAG, "the gatts demo char length = %x\n", length);
        for(int i = 0; i < length; i++){
            ESP_LOGI(BLES_TAG, "prf_char[%x] =%x\n",i,prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(app_profiles[idx].service_handle, &app_profiles[idx].descr_uuid,
                                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                                NULL, NULL);
        if (add_descr_ret){
            ESP_LOGE(BLES_TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        app_profiles[idx].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(BLES_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(BLES_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(BLES_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        app_profiles[idx].conn_id = param->connect.conn_id;

        if (idx == PROFILE_1_APP_ID)
        {
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
        }
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(BLES_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(BLES_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(BLES_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void esp_gatts_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            ble_server.app_profiles[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(BLES_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == ble_server.app_profiles[idx].gatts_if) {
                if (ble_server.app_profiles[idx].gatts_cb) {
                    ble_server.app_profiles[idx].gatts_cb(event, gatts_if, param, idx);
                }
            }
        }
    } while (0);
}

/* API Globals */
void bt_setup(void) 
{
    esp_err_t ret;
    /* Initialize BT Controller */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BLES_TAG, "%s Initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /* Bluetooth Controller Enable */
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BLES_TAG, "%s Enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /* Initialize Bluedroid API Stack */
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(BLES_TAG, "%s Init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /* Enable Bluedroid API Stack */
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BLES_TAG, "%s Enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
}

void ble_register_cbs(void) 
{
    esp_err_t ret;
    /* Register the  callback function to the gap module */
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret) {
        ESP_LOGE(BLES_TAG, "GAP register error, error code = %x", ret);
        return;
    }

    /* Register the callback function to the GATTS module */
    ret = esp_ble_gatts_register_callback(esp_gatts_cb);
    if(ret) {
        ESP_LOGE(BLES_TAG, "GATTS register error, error code = %x", ret);
        return;
    }
}

void ble_setup(void) 
{
    esp_err_t ret;
    /* Initialize NVS Flash module - Non-Volatile Storage */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    /* Realease Memory for BT Controller */
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /* Call BT Controller & Bluedroid Stack API */
    bt_setup();

    /* Register BLE GAP & BLE GATT Server Callbacks */
    ble_register_cbs();

}

void ble_register_app(void)
{
    esp_err_t ret = ESP_FAIL;
    /* Register number of profiles to be used in the app */
    /* This will trigger ESP_GATTS_REG_EVT */
    for (uint8_t i = 0; i < PROFILE_NUM; i++)
    {
        profiles_app_id_t app_id = (profiles_app_id_t)i;
        /* Use index as argument to setup app register. */
        ret = esp_ble_gatts_app_register(app_id);
        if (ret) {
            ESP_LOGE(BLES_TAG, "GATTS app register error, error code = %x", ret);
            ESP_LOGE(BLES_TAG, "Failed to register APP_ID: %d", app_id);
            return;
        }
    }
    /* Once completed, ESP_GATTS_REG_EVT will trigger ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT automatically. */
    /* ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT could trigger ble_start_scan. */
}

void ble_set_local_mtu(uint16_t mtu)
{
    /* Run after starting BLE scan */
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(mtu);
    if (local_mtu_ret){
        ESP_LOGE(BLES_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    } else {
        // ESP_LOGE(BLES_TAG, "set local  MTU sucess, code = %x", local_mtu_ret);
    }
}


void app_main(void)
{
    /* Run complete BLE setup */
    ble_setup();

    /* Register BLE App Profiles */
    ble_register_app();

    /* Setup MTU size */
    ble_set_local_mtu(500);

    return;
}
