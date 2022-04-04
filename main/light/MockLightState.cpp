//
// Created by bartosz on 3/3/22.
//

#include "MockLightState.h"

#include "main.h"


MockLightState::MockLightState(SmartLightFSM* fsm)
		: SmartLightState(fsm), m_ledRing(LED_RING_LED_COUNT, LED_RING_OUT_PIN) {}

void MockLightState::begin() {
	m_ledRing.initialize();
	m_ledRing.setColor(64, 0, 16);
	m_ledRing.turnOn();

	gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_OUTPUT);
}

void MockLightState::loop() {
	gpio_set_level(LED_GREEN_PIN, m_statusLedState);
	m_statusLedState = 1 - m_statusLedState;
	vTaskDelay(1000 / portTICK_PERIOD_MS);
}
