//
// Created by bartosz on 3/3/22.
//

#include <cstring>
#include "BleSetupState.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "main.h"
#include "setup.h"
#include "SetupStorage.h"
#include "mqtt/MqttSlaveState.h"
#include "tcp/TcpSlaveState.h"
#include "wifi/WiFi.h"


#define BLE_LOG_TAG "SMART_LIGHT_GATTS_TABLE"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x69
#define BLE_DEVICE_NAME          "Smart Light"

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
*/
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done = 0;
static uint16_t connection_id;
static bool is_connected = false;

//uint16_t smart_light_handle_table[SETUP_IDX_COUNT + MQTT_IDX_COUNT];

static SetupData setupData = {
	.wifiSsid = {},
	.wifiPasswd = {},
	.mode = SM_MODE_MQTT,
	.controlPoint = SETUP_IN_PROGRESS
};

static MqttConfig mqttData = {
	.username = {},
	.passwd = {},
	.brokerIp = {},
	.brokerPort = 1883
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
	.set_scan_rsp        = false,
	.include_name        = true,
	.include_txpower     = true,
	.min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
	.max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
	.appearance          = 0x00,
	.manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
	.p_manufacturer_data = NULL, //test_manufacturer,
	.service_data_len    = 0,
	.p_service_data      = NULL,
	.service_uuid_len    = 0,
	.p_service_uuid      = NULL,
	.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
	.adv_int_min         = 0x20,
	.adv_int_max         = 0x40,
	.adv_type            = ADV_TYPE_IND,
	.own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
	.channel_map         = ADV_CHNL_ALL,
	.adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
	esp_gatts_cb_t gatts_cb;
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
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
										esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst smart_light_profile_tab[PROFILE_NUM] = {
	[PROFILE_APP_IDX] = {
		.gatts_cb = gatts_profile_event_handler,
		.gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
	},
};

static uint8_t setup_uuid_base[16] = {
	0x0d, 0x86, 0x31, 0xef, 0xf7, 0x8d, 0xde, 0xb7,
	0x23, 0x49, 0xed, 0x88, 0xc2, 0xba, 0xe7, 0xb8
};

static uint8_t mqtt_uuid_base[16] = {
	0x77, 0xef, 0xf9, 0x20, 0x1d, 0x69, 0xb7, 0xbe,
	0x86, 0x47, 0x81, 0x64, 0x36, 0x11, 0x59, 0x10
};

#define MAKE_UUID_128(base, id) { \
        base[0], base[1], base[2], base[3], base[4], base[5], base[6], base[7], \
        base[8], base[9], base[10], base[11], id & 0xFF, (id >> 8) & 0xFF, base[14], base[15] \
}

static uint8_t setup_service_uuid[16] = MAKE_UUID_128(setup_uuid_base, SETUP_IDX_SVC);
static uint8_t wifi_ssid_char_uuid[16] = MAKE_UUID_128(setup_uuid_base, SETUP_IDX_CHAR_WIFI_SSID);
static uint8_t wifi_passwd_char_uuid[16] = MAKE_UUID_128(setup_uuid_base, SETUP_IDX_CHAR_WIFI_PASSWD);
static uint8_t mode_char_uuid[16] = MAKE_UUID_128(setup_uuid_base, SETUP_IDX_CHAR_MODE);
static uint8_t control_point_char_uuid[16] = MAKE_UUID_128(setup_uuid_base, SETUP_IDX_CHAR_CONTROL_POINT);

static uint8_t mqtt_service_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_SVC);
static uint8_t mqtt_username_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_USERNAME);
static uint8_t mqtt_passwd_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_PASSWD);
static uint8_t mqtt_broker_ip_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_BROKER_IP);
static uint8_t mqtt_broker_port_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_BROKER_PORT);
static uint8_t mqtt_dev_name_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_DEVICE_NAME);
static uint8_t mqtt_dev_group_char_uuid[16] = MAKE_UUID_128(mqtt_uuid_base, MQTT_IDX_CHAR_DEVICE_GROUP);

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify =
	ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t setup_cp_ccc[2] = { 0x00, 0x00 };

static GattServiceData smartLightGattData[SL_BLE_SVC_COUNT] = {{},
															   {}};


/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t setup_attr_tab[SETUP_IDX_COUNT] =
	{
		// Service Declaration
		[SETUP_IDX_SVC] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &primary_service_uuid, ESP_GATT_PERM_READ,
				 ESP_UUID_LEN_128, ESP_UUID_LEN_128, setup_service_uuid }},

		/* Characteristic Declaration */
		[SETUP_IDX_CHAR_WIFI_SSID] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[SETUP_IDX_CHAR_VAL_WIFI_SSID] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, wifi_ssid_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 WIFI_CRED_MAX_LEN, WIFI_CRED_MAX_LEN, (uint8_t*) setupData.wifiSsid }},

		/* Characteristic Declaration */
		[SETUP_IDX_CHAR_WIFI_PASSWD] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[SETUP_IDX_CHAR_VAL_WIFI_PASSWD] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, wifi_passwd_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 WIFI_CRED_MAX_LEN, WIFI_CRED_MAX_LEN, (uint8_t*) setupData.wifiPasswd }},

		/* Characteristic Declaration */
		[SETUP_IDX_CHAR_MODE] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[SETUP_IDX_CHAR_VAL_MODE] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mode_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 sizeof(setupData.mode), sizeof(setupData.mode), &setupData.mode }},

		/* Characteristic Declaration */
		[SETUP_IDX_CHAR_CONTROL_POINT] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write_notify }},

		/* Characteristic Value */
		[SETUP_IDX_CHAR_VAL_CONTROL_POINT] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, control_point_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 sizeof(setupData.controlPoint), sizeof(setupData.controlPoint), &setupData.controlPoint }},

		/* Client Characteristic Configuration Descriptor */
		[SETUP_IDX_CHAR_CFG_CONTROL_POINT] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_client_config_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 sizeof(uint16_t), sizeof(setup_cp_ccc), (uint8_t*) setup_cp_ccc }}
	};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t mqtt_attr_tab[MQTT_IDX_COUNT] =
	{
		[MQTT_IDX_SVC] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &primary_service_uuid, ESP_GATT_PERM_READ,
				 ESP_UUID_LEN_128, ESP_UUID_LEN_128, mqtt_service_uuid }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_USERNAME] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_USERNAME] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_username_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 MQTT_CRED_MAX_LEN, MQTT_CRED_MAX_LEN, (uint8_t*) mqttData.username }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_PASSWD] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_PASSWD] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_passwd_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 MQTT_CRED_MAX_LEN, MQTT_CRED_MAX_LEN, (uint8_t*) mqttData.passwd }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_BROKER_IP] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_BROKER_IP] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_broker_ip_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 IPV4_LEN, IPV4_LEN, mqttData.brokerIp }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_BROKER_PORT] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_BROKER_PORT] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_broker_port_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 sizeof(mqttData.brokerPort), sizeof(mqttData.brokerPort), (uint8_t*) &mqttData.brokerPort }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_DEVICE_NAME] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_DEVICE_NAME] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_dev_name_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 MQTT_CRED_MAX_LEN, MQTT_CRED_MAX_LEN, (uint8_t*) mqttData.deviceName }},

		/* Characteristic Declaration */
		[MQTT_IDX_CHAR_DEVICE_GROUP] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
				 CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write }},

		/* Characteristic Value */
		[MQTT_IDX_CHAR_VAL_DEVICE_GROUP] =
			{{ ESP_GATT_AUTO_RSP },
			 { ESP_UUID_LEN_128, mqtt_dev_group_char_uuid,
				 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
				 MQTT_CRED_MAX_LEN, MQTT_CRED_MAX_LEN, (uint8_t*) mqttData.deviceGroup }},
	};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	switch (event) {
		case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			adv_config_done &= (~ADV_CONFIG_FLAG);
			if (adv_config_done == 0) {
				esp_ble_gap_start_advertising(&adv_params);
			}
			break;
		case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
			adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
			if (adv_config_done == 0) {
				esp_ble_gap_start_advertising(&adv_params);
			}
			break;
		case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
			/* advertising start complete event to indicate advertising start successfully or failed */
			if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
				ESP_LOGE(BLE_LOG_TAG, "advertising start failed");
			}
			else {
				ESP_LOGI(BLE_LOG_TAG, "advertising start successfully");
			}
			break;
		case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
			if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
				ESP_LOGE(BLE_LOG_TAG, "Advertising stop failed");
			}
			else {
				ESP_LOGI(BLE_LOG_TAG, "Stop adv successfully\n");
			}
			break;
		case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
			ESP_LOGI(BLE_LOG_TAG,
					 "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
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

static void handle_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	for (int i = 0; i < SL_BLE_SVC_COUNT; i++) {
		auto svcGattData = &smartLightGattData[i];
		for (int j = 0; j < svcGattData->attrCount; j++) {
			if (svcGattData->data[j] && param->write.handle == svcGattData->handles[j]) {
				memcpy(svcGattData->data[j], param->write.value, param->write.len);
				ESP_LOGI(BLE_LOG_TAG, "Savied data for %u attr of service %u", j, i);
			}
		}
	}

	if (param->write.need_rsp) {
		esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
	}
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
										esp_ble_gatts_cb_param_t* param) {
	switch (event) {
		case ESP_GATTS_REG_EVT: {
			esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
			if (set_dev_name_ret) {
				ESP_LOGE(BLE_LOG_TAG, "set device name failed, error code = %x", set_dev_name_ret);
			}

			//config adv data
			esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
			if (ret) {
				ESP_LOGE(BLE_LOG_TAG, "config adv data failed, error code = %x", ret);
			}
			adv_config_done |= ADV_CONFIG_FLAG;

			//config scan response data
			adv_data.set_scan_rsp = true;
			ret = esp_ble_gap_config_adv_data(&adv_data);
			if (ret) {
				ESP_LOGE(BLE_LOG_TAG, "config scan response data failed, error code = %x", ret);
			}
			adv_config_done |= SCAN_RSP_CONFIG_FLAG;

			esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(
				setup_attr_tab, gatts_if, SETUP_IDX_COUNT, SL_BLE_SVC_SETUP
			);
			if (create_attr_ret) {
				ESP_LOGE(BLE_LOG_TAG, "create setup attr table failed, error code = %x", create_attr_ret);
			}

			create_attr_ret = esp_ble_gatts_create_attr_tab(
				mqtt_attr_tab, gatts_if, MQTT_IDX_COUNT, SL_BLE_SVC_MQTT
			);
			if (create_attr_ret) {
				ESP_LOGE(BLE_LOG_TAG, "create mqtt attr table failed, error code = %x", create_attr_ret);
			}
		}
			break;
		case ESP_GATTS_READ_EVT:
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_READ_EVT");
			break;
		case ESP_GATTS_WRITE_EVT:
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_WRITE_EVT");
			handle_write_event(gatts_if, param);
			break;
		case ESP_GATTS_MTU_EVT:
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
			break;
		case ESP_GATTS_CONF_EVT:
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status,
					 param->conf.handle);
			break;
		case ESP_GATTS_START_EVT:
			ESP_LOGI(BLE_LOG_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status,
					 param->start.service_handle);
			break;
		case ESP_GATTS_CONNECT_EVT: {
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
				esp_log_buffer_hex(BLE_LOG_TAG, param->connect.remote_bda, 6);
			esp_ble_conn_update_params_t conn_params = {};
			memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
			/* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
			conn_params.latency = 0;
			conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
			conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
			conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
			//start sent the update connection parameters to the peer device.
			esp_ble_gap_update_conn_params(&conn_params);

			is_connected = true;
			connection_id = param->connect.conn_id;
			break;
		}
		case ESP_GATTS_DISCONNECT_EVT:
			ESP_LOGI(BLE_LOG_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
			if (param->disconnect.reason != ESP_GATT_CONN_TERMINATE_LOCAL_HOST) {
				esp_ble_gap_start_advertising(&adv_params);
			}
			is_connected = false;
			break;
		case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
			if (param->add_attr_tab.status != ESP_GATT_OK) {
				ESP_LOGE(BLE_LOG_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
				return;
			}

			ESP_LOGI(
				BLE_LOG_TAG, "create attribute table successfully, service id = %u, attr count = %d\n",
				param->add_attr_tab.svc_inst_id, param->add_attr_tab.num_handle
			);

			auto svcGattData = &smartLightGattData[param->add_attr_tab.svc_inst_id];
			svcGattData->attrCount = param->add_attr_tab.num_handle;
			memcpy(
				svcGattData->handles, param->add_attr_tab.handles,
				param->add_attr_tab.num_handle * sizeof(uint16_t)
			);

			if (param->add_attr_tab.svc_inst_id == SL_BLE_SVC_SETUP) {
				svcGattData->data[SETUP_IDX_CHAR_VAL_WIFI_SSID] = (uint8_t*) setupData.wifiSsid;
				svcGattData->data[SETUP_IDX_CHAR_VAL_WIFI_PASSWD] = (uint8_t*) setupData.wifiPasswd;
				svcGattData->data[SETUP_IDX_CHAR_VAL_MODE] = &setupData.mode;
				svcGattData->data[SETUP_IDX_CHAR_VAL_CONTROL_POINT] = &setupData.controlPoint;
			}

			if (param->add_attr_tab.svc_inst_id == SL_BLE_SVC_MQTT) {
				svcGattData->data[MQTT_IDX_CHAR_VAL_USERNAME] = (uint8_t*) mqttData.username;
				svcGattData->data[MQTT_IDX_CHAR_VAL_PASSWD] = (uint8_t*) mqttData.passwd;
				svcGattData->data[MQTT_IDX_CHAR_VAL_BROKER_IP] = mqttData.brokerIp;
				svcGattData->data[MQTT_IDX_CHAR_VAL_BROKER_PORT] = (uint8_t*) &mqttData.brokerPort;
				svcGattData->data[MQTT_IDX_CHAR_VAL_DEVICE_NAME] = (uint8_t*) mqttData.deviceName;
				svcGattData->data[MQTT_IDX_CHAR_VAL_DEVICE_GROUP] = (uint8_t*) mqttData.deviceGroup;
			}

			esp_ble_gatts_start_service(svcGattData->handles[SETUP_IDX_SVC]);
			break;
		}
		case ESP_GATTS_STOP_EVT:
		case ESP_GATTS_OPEN_EVT:
		case ESP_GATTS_CANCEL_OPEN_EVT:
		case ESP_GATTS_CLOSE_EVT:
		case ESP_GATTS_LISTEN_EVT:
		case ESP_GATTS_CONGEST_EVT:
		case ESP_GATTS_UNREG_EVT:
		case ESP_GATTS_DELETE_EVT:
		default:
			break;
	}
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {
	/* If event is register event, store the gatts_if for each profile */
	if (event == ESP_GATTS_REG_EVT) {
		if (param->reg.status == ESP_GATT_OK) {
			smart_light_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
		}
		else {
			ESP_LOGE(BLE_LOG_TAG, "reg app failed, app_id %04x, status %d",
					 param->reg.app_id,
					 param->reg.status);
			return;
		}
	}
	int idx;
	for (idx = 0; idx < PROFILE_NUM; idx++) {
		/* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
		if (gatts_if == ESP_GATT_IF_NONE || gatts_if == smart_light_profile_tab[idx].gatts_if) {
			if (smart_light_profile_tab[idx].gatts_cb) {
				smart_light_profile_tab[idx].gatts_cb(event, gatts_if, param);
			}
		}
	}
}


BleSetupState::BleSetupState(SmartLightFSM* fsm)
	: SmartLightState(fsm) {}

void BleSetupState::begin() {
	gpio_set_level(LED_RED_PIN, 1);

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_err_t ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return;
	}

	ret = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "gatts register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gap_register_callback(gap_event_handler);
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "gap register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gatts_app_register(ESP_APP_ID);
	if (ret) {
		ESP_LOGE(BLE_LOG_TAG, "gatts app register error, error code = %x", ret);
		return;
	}

	esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
	if (local_mtu_ret) {
		ESP_LOGE(BLE_LOG_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}
}

void BleSetupState::loop() {
	if (setupData.controlPoint == SETUP_READY) {
		ESP_LOGI(BLE_LOG_TAG, "Setup ready, connecting to WiFi %s", setupData.wifiSsid);

		setupData.controlPoint = SETUP_WIFI_CONNECTING;
		esp_ble_gatts_send_indicate(
			smart_light_profile_tab[PROFILE_APP_IDX].gatts_if, connection_id,
			smartLightGattData[SL_BLE_SVC_SETUP].handles[SETUP_IDX_CHAR_VAL_CONTROL_POINT],
			sizeof(setupData.controlPoint), &setupData.controlPoint, false
		);

		wifiInit();
		wifiConnect(setupData.wifiSsid, setupData.wifiPasswd);

		setupData.controlPoint = SETUP_WIFI_CONNECTED;
		esp_ble_gatts_send_indicate(
			smart_light_profile_tab[PROFILE_APP_IDX].gatts_if, connection_id,
			smartLightGattData[SL_BLE_SVC_SETUP].handles[SETUP_IDX_CHAR_VAL_CONTROL_POINT],
			sizeof(setupData.controlPoint), &setupData.controlPoint, false
		);
	}
	else if (setupData.controlPoint == SETUP_DONE) {
		ESP_LOGI(BLE_LOG_TAG, "Setup done, turning off BLE");

		if (is_connected) {
			ESP_LOGI(BLE_LOG_TAG, "Disconnecting current client");

			esp_err_t disconnect_result = esp_ble_gatts_close(
				smart_light_profile_tab[PROFILE_APP_IDX].gatts_if, connection_id
			);
			if (disconnect_result != ESP_OK) {
				ESP_LOGW(BLE_LOG_TAG, "Failed to disconnect client");
			}
		}

		ESP_ERROR_CHECK(esp_bluedroid_disable());
		ESP_ERROR_CHECK(esp_bluedroid_deinit());

		ESP_ERROR_CHECK(esp_bt_controller_disable());
		ESP_ERROR_CHECK(esp_bt_controller_deinit());

		auto& storage = SetupStorage::get();
		storage.storeSetup(&setupData);
		storage.storeModeSetup((SmartLightMode) setupData.mode, &mqttData, sizeof(mqttData));

		gpio_set_level(LED_RED_PIN, 0);

		if (setupData.mode == SM_MODE_TCP) {
			m_fsm->setState(new TcpSlaveState(m_fsm));
		}
		else {
			m_fsm->setState(new MqttSlaveState(m_fsm, mqttData));
		}
	}
	else {
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
