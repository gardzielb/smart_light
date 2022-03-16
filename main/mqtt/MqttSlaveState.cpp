//
// Created by bartosz on 3/16/22.
//

#include "MqttSlaveState.h"

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MQTT_SLAVE_TAG "MQQT_SLAVE"


MqttSlaveState::MqttSlaveState(SmartLightFSM* fsm) : SmartLightState(fsm) {}

void MqttSlaveState::begin() {
	ESP_LOGI(MQTT_SLAVE_TAG, "Entering MQTT slave state");
}

void MqttSlaveState::loop() {
	ESP_LOGI(MQTT_SLAVE_TAG, "MQTT slave state loop");
	vTaskDelay(2000 / portTICK_PERIOD_MS);
}
