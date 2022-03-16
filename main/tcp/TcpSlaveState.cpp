//
// Created by bartosz on 3/16/22.
//

#include "TcpSlaveState.h"

#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TCP_SLAVE_TAG "TCP_SLAVE"


TcpSlaveState::TcpSlaveState(SmartLightFSM* fsm) : SmartLightState(fsm) {}

void TcpSlaveState::begin() {
	ESP_LOGI(TCP_SLAVE_TAG, "Entering TCP slave state");
}

void TcpSlaveState::loop() {
	ESP_LOGI(TCP_SLAVE_TAG, "TCP slave state loop");
	vTaskDelay(2000 / portTICK_PERIOD_MS);
}
