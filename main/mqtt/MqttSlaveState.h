//
// Created by bartosz on 3/16/22.
//

#ifndef SMART_LIGHT_MQTTSLAVESTATE_H
#define SMART_LIGHT_MQTTSLAVESTATE_H

#include "stdint.h"
#include "mqtt_client.h"

#include "SmartLightFSM.h"
#include "LedRing.h"

#define IPV4_LEN 4
#define MQTT_CRED_MAX_LEN 20


struct MqttConfig {
	char username[MQTT_CRED_MAX_LEN];
	char passwd[MQTT_CRED_MAX_LEN];
	uint8_t brokerIp[IPV4_LEN];
	uint16_t brokerPort;
	char deviceName[MQTT_CRED_MAX_LEN];
	char deviceGroup[MQTT_CRED_MAX_LEN];
};


enum LightCommand {
	LIGHT_IDLE,
	LIGHT_ON,
	LIGHT_OFF
};


class MqttSlaveState : public SmartLightState {
public:
	MqttSlaveState(SmartLightFSM* fsm, MqttConfig config);

	void begin() override;

	void loop() override;

	void onMqttConnected(esp_mqtt_client_handle_t mqttClient);

	inline void setCommand(LightCommand command) {
		m_command = command;
	}

private:
	MqttConfig m_config;
	LedRing m_ledRing;
	LightCommand m_command = LightCommand::LIGHT_IDLE;
};


#endif //SMART_LIGHT_MQTTSLAVESTATE_H
