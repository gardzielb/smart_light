//
// Created by bartosz on 3/16/22.
//

#ifndef SMART_LIGHT_MQTTSLAVESTATE_H
#define SMART_LIGHT_MQTTSLAVESTATE_H

#include "stdint.h"
#include "mqtt_client.h"

#include "SmartLightFSM.h"
#include "light/LedRing.h"

#define IPV4_LEN 4
#define MQTT_CRED_MAX_LEN 20
#define MQTT_MAX_MSG_SIZE 64
#define MQTT_PING_REQUEST_TOPIC "/smart_light/ping/request"
#define MQTT_PING_RESPONSE_TOPIC "/smart_light/ping/response"


struct MqttConfig {
	char username[MQTT_CRED_MAX_LEN];
	char passwd[MQTT_CRED_MAX_LEN];
	uint8_t brokerIp[IPV4_LEN];
	uint16_t brokerPort;
	char deviceName[MQTT_CRED_MAX_LEN];
	char deviceGroup[MQTT_CRED_MAX_LEN];
};


class MqttSlaveState : public SmartLightState {
public:
	MqttSlaveState(SmartLightFSM* fsm, MqttConfig config);

	void begin() override;

	void loop() override;

	void onMqttConnected(esp_mqtt_client_handle_t mqttClient);

	void receiveMessage(uint8_t* data, uint32_t dataLen);

	~MqttSlaveState();

private:
	void sendPing();

	MqttConfig m_config;
	uint8_t m_cmdBuffer[MQTT_MAX_MSG_SIZE] = {};
	uint32_t m_cmdBytesCount = 0;
	esp_mqtt_client_handle_t m_mqttClient;
};


#endif //SMART_LIGHT_MQTTSLAVESTATE_H
