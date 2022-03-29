//
// Created by bartosz on 3/16/22.
//

#include "MqttSlaveState.h"

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LOGGER_TAG "MQQT_SLAVE"
#define LED_RING_PIN GPIO_NUM_5
#define LED_RING_LED_COUNT 12


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

			if (!strncmp(event->data, "on", event->data_len)) {
				ESP_LOGI(LOGGER_TAG, "Light on");
				mqttState->setCommand(LightCommand::LIGHT_ON);
			}
			else if (!strncmp(event->data, "off", event->data_len)) {
				ESP_LOGI(LOGGER_TAG, "Light off");
				mqttState->setCommand(LightCommand::LIGHT_OFF);
			}

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
	: SmartLightState(fsm), m_config(config), m_ledRing(LED_RING_LED_COUNT, LED_RING_PIN) {}

void MqttSlaveState::begin() {
	ESP_LOGI(LOGGER_TAG, "Entering MQTT slave state");

	m_ledRing.initialize();
	m_ledRing.setColor(64, 0, 16);
	m_ledRing.turnOn();

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

	esp_mqtt_client_handle_t mqttClient = esp_mqtt_client_init(&mqttClientConfig);
	esp_mqtt_client_register_event(
		mqttClient, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), mqttEventHandler, this
	);
	esp_mqtt_client_start(mqttClient);
}

void MqttSlaveState::loop() {
	switch (m_command) {
		case LIGHT_IDLE:
			vTaskDelay(100 / portTICK_PERIOD_MS);
			break;
		case LIGHT_ON:
			m_command = LIGHT_IDLE;
			m_ledRing.turnOn();
			break;
		case LIGHT_OFF:
			m_command = LIGHT_IDLE;
			m_ledRing.turnOff();
			break;
	}
}

void MqttSlaveState::onMqttConnected(esp_mqtt_client_handle_t mqttClient) {
	int msg_id = esp_mqtt_client_publish(mqttClient, "/smart_light/devices", "hello there", 0, 0, 0);
	ESP_LOGI(LOGGER_TAG, "sent publish successful, msg_id=%d", msg_id);

	char topic[13 + MQTT_CRED_MAX_LEN + 1] = {};
	sprintf(topic, "/smart_light/%s", m_config.deviceName);
	msg_id = esp_mqtt_client_subscribe(mqttClient, topic, 0);
	ESP_LOGI(LOGGER_TAG, "sent subscribe successful, msg_id=%d", msg_id);

	memset(topic, 0, sizeof(topic));
	sprintf(topic, "/smart_light/%s", m_config.deviceGroup);
	msg_id = esp_mqtt_client_subscribe(mqttClient, topic, 0);
	ESP_LOGI(LOGGER_TAG, "sent subscribe successful, msg_id=%d", msg_id);
}
