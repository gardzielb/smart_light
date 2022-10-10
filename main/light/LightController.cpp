//
// Created by bartosz on 4/1/22.
//

#include "LightController.h"
#include "setup/BleSetupState.h"
#include "BlinkState.h"
#include "setup/SetupStorage.h"

#include <esp_log.h>

#define LOG_TAG "LightController"
#define FADING_PERIOD_MS 200.f


static void fadingTimerCallback(TimerHandle_t timerHandle) {
	FadingData* fadingData = (FadingData*) pvTimerGetTimerID(timerHandle);
	float alpha = fadingData->light->getIntensity() - fadingData->alphaStep;

	if (alpha >= 0.f) {
		fadingData->light->setIntensity(alpha);
	}
	else {
		fadingData->light->turnOff();
		fadingData->light->setIntensity(fadingData->baseAlpha);
		xTimerStopFromISR(timerHandle, 0);
	}
}

static void operationTimerCallback(TimerHandle_t timerHandle) {
	LightController::get().executePendingOperation();
}


void LightController::startFading(uint16_t fadingDuration, bool fromIsr) {
	m_fadingData.baseAlpha = m_light.getIntensity();
	uint32_t fadingDurationMs = fadingDuration * 1000;
	m_fadingData.alphaStep = m_fadingData.baseAlpha * FADING_PERIOD_MS / (float) fadingDurationMs;

	ESP_LOGI(
		LOG_TAG,
		"Starting fade out process with duration = %u, base alpha = %f, alpha step = %f",
		fadingDuration, m_fadingData.baseAlpha, m_fadingData.alphaStep
	);

	if (fromIsr)
		xTimerStartFromISR(m_fadingData.timerHandle, 0);
	else
		xTimerStart(m_fadingData.timerHandle, 0);
}

void LightController::handleOperation(SmartLightOperation* operation, SmartLightFSM* slFsm) {
	if (operation->delay) {
		memcpy(&m_pendingOperation, operation, sizeof(SmartLightOperation));

		if (m_opTimerHandle) {
			xTimerStop(m_opTimerHandle, 0);
			xTimerDelete(m_opTimerHandle, 0);
		}

		m_opTimerHandle = xTimerCreate(
			"light op timer",
			1000 * operation->delay / portTICK_PERIOD_MS,
			pdFALSE,
			NULL,
			operationTimerCallback
		);

		xTimerStart(m_opTimerHandle, 0);
	}
	else {
		execute(operation, slFsm);
	}
}

uint8_t LightController::executeCommand(uint8_t cmd, uint8_t* args, SmartLightFSM* slFsm, bool fromIsr) {
	if (cmd == SL_SETUP) {
		SetupStorage::get().clear();
		slFsm->restart();
	}

//	ESP_LOGI(
//		LOG_TAG, "Executing command %u with delay = %u and data = [%u, %u, %u, %u, %u]",
//		operation->command, operation->delay, operation->data[0], operation->data[1],
//		operation->data[2], operation->data[3], operation->data[4]
//	);

	switch (cmd) {
		case SL_IDLE:
			return 0;
		case SL_ON:
			m_light.turnOn();
			return 0;
		case SL_OFF:
			m_light.turnOff();
			return 0;
		case SL_SET_COLOR:
			ESP_LOGI(LOG_TAG, "Setting color to (%u, %u, %u)", args[0], args[1], args[2]);
			m_light.setColor(args[0], args[1], args[2]);
			return 3;
		case SL_SET_INTENSITY:
			ESP_LOGI(LOG_TAG, "Setting intensity to %u", args[0]);
			m_light.setIntensity(args[0] / 100.f);
			return 1;
		case SL_FADE_OUT:
			startFading(*(uint16_t*) args, fromIsr);
			return 2;
		case SL_AUTO:
			// TODO
			return 0;
		default:
			ESP_LOGW(LOG_TAG, "Attempting to execute unexpected operation %u", cmd);
			return 0;
	}
}

void LightController::execute(SmartLightOperation* operation, SmartLightFSM* slFsm, bool fromIsr) {
	int i = 0;
	while (i < operation->dataLen) {
		uint8_t argsLen = executeCommand(operation->data[i], &operation->data[i + 1], slFsm, fromIsr);
		i += (argsLen + 1);
	}
}

void LightController::executePendingOperation() {
	execute(&m_pendingOperation.operation, m_pendingOperation.slFsm, true);
}

LightController::LightController()
	: m_light(LED_RING_LED_COUNT, LED_RING_OUT_PIN) {
	m_fadingData.timerHandle = xTimerCreate(
		"fading timer",
		FADING_PERIOD_MS / portTICK_PERIOD_MS,
		pdTRUE,
		&m_fadingData,
		fadingTimerCallback
	);
}

LightController::~LightController() {
	if (m_opTimerHandle) {
		xTimerStop(m_opTimerHandle, 0);
		xTimerDelete(m_opTimerHandle, 0);
	}

	if (m_fadingData.timerHandle) {
		xTimerStop(m_opTimerHandle, 0);
		xTimerDelete(m_opTimerHandle, 0);
	}
}
