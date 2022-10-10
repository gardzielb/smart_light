//
// Created by bartosz on 3/16/22.
//

#include "MqttSlaveState.h"

#include <esp_log.h>
#include <cJSON.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main.h"
#include "light/LightController.h"

#define LOGGER_TAG "MQTT_SLAVE"


static void log_error_if_nonzero(const char* message, int error_code) {
	if (error_code != 0) {
		ESP_LOGE(LOGGER_TAG, "Last error %s: 0x%x", message, error_code);
	}
}

static void mqttEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventId, void* eventData) {
	ESP_LOGD(LOGGER_TAG, "Event dispatched from event loop base=%s, event_id=%d", base, eventId);

	esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(eventData);
	MqttSlaveState* mqttState = (MqttSlaveState*) handlerArgs;

	switch ((esp_mqtt_event_id_t) eventId) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_CONNECTED");
			mqttState->onMqttConnected(event->client);
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_DISCONNECTED");
			break;

		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_DATA");
			ESP_LOGI(
				LOGGER_TAG, "TOPIC=%.*s, DATA=%.*s\n", event->topic_len, event->topic, event->data_len, event->data
			);
			mqttState->receiveMessage((uint8_t*) event->data, event->data_len);
			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGI(LOGGER_TAG, "MQTT_EVENT_ERROR");
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
				log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
				log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
				log_error_if_nonzero(
					"captured as transport's socket errno", event->error_handle->esp_transport_sock_errno
				);
				ESP_LOGI(LOGGER_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
			}
			break;

		default:
			ESP_LOGI(LOGGER_TAG, "Other event id:%d", event->event_id);
			break;
	}
}


MqttSlaveState::MqttSlaveState(SmartLightFSM* fsm, MqttConfig config)
	: SmartLightState(fsm), m_config(config) {}

void MqttSlaveState::begin() {
	ESP_LOGI(LOGGER_TAG, "Entering MQTT slave state");
	gpio_set_level(LED_GREEN_PIN, 1);

	char brokerIpStr[IPV4_LEN * 4] = {};
	sprintf(
		brokerIpStr, "%u.%u.%u.%u",
		m_config.brokerIp[0], m_config.brokerIp[1], m_config.brokerIp[2], m_config.brokerIp[3]
	);

	esp_mqtt_client_config_t mqttClientConfig = {
		.host = brokerIpStr,
		.port = m_config.brokerPort,
		.username = m_config.username,
		.password = m_config.passwd
	};

	m_mqttClient = esp_mqtt_client_init(&mqttClientConfig);
	esp_mqtt_client_register_event(
		m_mqttClient, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqttEventHandler, this
	);
	esp_mqtt_client_start(m_mqttClient);
}

void MqttSlaveState::loop() {
	if (!m_cmdBytesCount) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
		return;
	}

	auto& lightController = LightController::get();

	switch (m_cmdBuffer[0]) {
		case SL_JSON:
			handleJsonMsg(lightController);
			break;
		case SL_PING:
			sendPing();
			break;
		default:
			lightController.handleOperation((SmartLightOperation*) m_cmdBuffer, m_fsm);
			break;
	}

	memset(m_cmdBuffer, 0, MQTT_MAX_MSG_SIZE);
	m_cmdBytesCount = 0;
}

void MqttSlaveState::onMqttConnected(esp_mqtt_client_handle_t mqttClient) {
	sendPing();

	char topic[13 + MQTT_CRED_MAX_LEN + 1] = {};
	sprintf(topic, "/smart_light/%s", m_config.deviceName);
	int msg_id = esp_mqtt_client_subscribe(mqttClient, topic, 0);
	ESP_LOGI(LOGGER_TAG, "sent subscribe successful, msg_id=%d", msg_id);

	memset(topic, 0, sizeof(topic));
	sprintf(topic, "/smart_light/%s", m_config.deviceGroup);
	msg_id = esp_mqtt_client_subscribe(mqttClient, topic, 0);
	ESP_LOGI(LOGGER_TAG, "sent subscribe successful, msg_id=%d", msg_id);

	msg_id = esp_mqtt_client_subscribe(mqttClient, MQTT_PING_REQUEST_TOPIC, 0);
	ESP_LOGI(LOGGER_TAG, "sent subscribe successful, msg_id=%d", msg_id);
}

MqttSlaveState::~MqttSlaveState() {
	esp_mqtt_client_disconnect(m_mqttClient);
	esp_mqtt_client_destroy(m_mqttClient);
	gpio_set_level(LED_GREEN_PIN, 0);
}

void MqttSlaveState::receiveMessage(uint8_t* data, uint32_t dataLen) {
	m_cmdBytesCount = dataLen;
	memcpy(m_cmdBuffer, data, dataLen);
}

void MqttSlaveState::sendPing() {
	int msg_id = esp_mqtt_client_publish(m_mqttClient, MQTT_PING_RESPONSE_TOPIC, m_config.deviceName, 0, 0, 0);
	ESP_LOGI(LOGGER_TAG, "sent publish successful, msg_id=%d", msg_id);
}

void MqttSlaveState::handleJsonMsg(LightController& lightController) {
	SmartLightOperation batchOperation = {};
	auto json = cJSON_Parse((const char*) m_cmdBuffer);

	if (cJSON_HasObjectItem(json, "delay")) {
		auto delayItem = cJSON_GetObjectItem(json, "delay");
		batchOperation.delay = cJSON_GetNumberValue(delayItem);
		ESP_LOGI(LOGGER_TAG, "delay = %u", batchOperation.delay);
	}

	uint8_t* dataPtr = batchOperation.data;

	if (cJSON_HasObjectItem(json, "on")) {
		auto onOffItem = cJSON_GetObjectItem(json, "on");
		uint8_t lightOn = cJSON_IsTrue(onOffItem);
		ESP_LOGI(LOGGER_TAG, "light on = %u", lightOn);
		*dataPtr = lightOn ? SL_ON : SL_OFF;
		dataPtr++;
	}

	if (cJSON_HasObjectItem(json, "color")) {
		dataPtr[0] = SL_SET_COLOR;

		auto colorItem = cJSON_GetObjectItem(json, "color");
		auto colorHex = cJSON_GetStringValue(colorItem);
		ESP_LOGI(LOGGER_TAG, "color = %s", colorHex);

		uint32_t rgb = (uint32_t) strtol(colorHex + 1, NULL, 16);
		dataPtr[1] = (rgb >> 16) & 0xFF;
		dataPtr[2] = (rgb >> 8) & 0xFF;
		dataPtr[3] = rgb & 0xFF;
		dataPtr += 4;
	}

	if (cJSON_HasObjectItem(json, "alpha")) {
		dataPtr[0] = SL_SET_INTENSITY;

		auto alphaItem = cJSON_GetObjectItem(json, "alpha");
		uint8_t alpha = cJSON_GetNumberValue(alphaItem);
		ESP_LOGI(LOGGER_TAG, "alpha = %u", alpha);

		dataPtr[1] = alpha;
		dataPtr += 2;
	}

	if (cJSON_HasObjectItem(json, "fade")) {
		auto fadeItem = cJSON_GetObjectItem(json, "fade");
		uint16_t fadeDuration = cJSON_GetNumberValue(fadeItem);
		ESP_LOGI(LOGGER_TAG, "fade = %u", fadeDuration);

		if (fadeDuration) {
			*dataPtr = SL_FADE_OUT;
			memcpy(dataPtr + 1, &fadeDuration, sizeof(fadeDuration));
			dataPtr += (1 + sizeof(fadeDuration));
		}
	}

	if (cJSON_HasObjectItem(json, "reset")) {
		auto setupItem = cJSON_GetObjectItem(json, "reset");
		bool exitToSetup = cJSON_IsTrue(setupItem);
		ESP_LOGI(LOGGER_TAG, "exit to setup mode = %u", exitToSetup);

		if (exitToSetup) {
			*dataPtr = SL_SETUP;
			dataPtr++;
		}
	}

	batchOperation.dataLen = dataPtr - batchOperation.data;

	cJSON_Delete(json);
	lightController.handleOperation(&batchOperation, m_fsm);
}
