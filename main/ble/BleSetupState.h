//
// Created by bartosz on 3/3/22.
//

#ifndef SMART_LIGHT_BLESETUPSTATE_H
#define SMART_LIGHT_BLESETUPSTATE_H

#include "SmartLightFSM.h"

#include "esp_gatt_common_api.h"


class BleSetupState : public SmartLightState {
public:
	BleSetupState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;
};


enum SmartLightMode {
	SM_MODE_MQTT,
	SM_MODE_TCP
};


enum SetupStatus {
	SETUP_IN_PROGRESS,
	SETUP_READY,
	SETUP_WIFI_CONNECTING,
	SETUP_WIFI_CONNECTED,
	SETUP_DONE
};


struct SetupData {
	char wifiSsid[ESP_GATT_DEF_BLE_MTU_SIZE];
	char wifiPasswd[ESP_GATT_DEF_BLE_MTU_SIZE];
	uint8_t mode;
	uint8_t controlPoint;
};

#define IPV4_ADDR_LEN 4

struct MqttData {
	char name[ESP_GATT_DEF_BLE_MTU_SIZE];
	char passwd[ESP_GATT_DEF_BLE_MTU_SIZE];
	uint8_t brokerIp[IPV4_ADDR_LEN];
	uint16_t brokerPort;
};


enum SetupServiceIndex {
	SETUP_IDX_SVC,

	SETUP_IDX_CHAR_WIFI_SSID,
	SETUP_IDX_CHAR_VAL_WIFI_SSID,

	SETUP_IDX_CHAR_WIFI_PASSWD,
	SETUP_IDX_CHAR_VAL_WIFI_PASSWD,

	SETUP_IDX_CHAR_MODE,
	SETUP_IDX_CHAR_VAL_MODE,

	SETUP_IDX_CHAR_CONTROL_POINT,
	SETUP_IDX_CHAR_VAL_CONTROL_POINT,
	SETUP_IDX_CHAR_CFG_CONTROL_POINT,

	SETUP_IDX_COUNT,
};

enum {
	MQTT_IDX_SVC,

	MQTT_IDX_CHAR_MQTT_NAME,
	MQTT_IDX_CHAR_VAL_MQTT_NAME,

	MQTT_IDX_CHAR_MQTT_PASSWD,
	MQTT_IDX_CHAR_VAL_MQTT_PASSWD,

	MQTT_IDX_CHAR_MQTT_BROKER_IP,
	MQTT_IDX_CHAR_VAL_MQTT_BROKER_IP,

	MQTT_IDX_CHAR_MQTT_BROKER_PORT,
	MQTT_IDX_CHAR_VAL_MQTT_BROKER_PORT,

	MQTT_IDX_COUNT,
};


#endif //SMART_LIGHT_BLESETUPSTATE_H
