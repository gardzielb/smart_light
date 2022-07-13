//
// Created by bartosz on 3/3/22.
//

#include "BlinkState.h"

#include "main.h"


BlinkState::BlinkState(SmartLightFSM* fsm) : SmartLightState(fsm) {}

void BlinkState::begin() {
	ESP_LOGI("BLINK", "Entering blink state");
}

void BlinkState::loop() {
	gpio_set_level(LED_RED_PIN, m_statusLedState);
	m_statusLedState = 1 - m_statusLedState;
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}
