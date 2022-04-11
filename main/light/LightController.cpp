//
// Created by bartosz on 4/1/22.
//

#include "LightController.h"
#include "setup/BleSetupState.h"

#include <esp_log.h>

#define LOG_TAG "LightController"


static void performLightOperation(SmartLightOperation* operation, LedRing* light) {
	ESP_LOGI(
		LOG_TAG, "Executing command %u with delay = %u and data = [%u, %u, %u, %u, %u]",
		operation->command, operation->delay, operation->data[0], operation->data[1],
		operation->data[2], operation->data[3], operation->data[4]
	);

	switch (operation->command) {
		case SL_IDLE:
			break;
		case SL_ON:
			light->turnOn();
			break;
		case SL_OFF:
			light->turnOff();
			break;
		case SL_SET_COLOR:
			ESP_LOGI(
				LOG_TAG, "Setting color to (%u, %u, %u)", operation->data[0], operation->data[1], operation->data[2]
			);
			light->setColor(operation->data[0], operation->data[1], operation->data[2]);
			break;
		case SL_SET_INTENSITY:
			ESP_LOGI(LOG_TAG, "Setting intensity to %u", operation->data[0]);
			light->setIntensity(operation->data[0] / 100.f);
			break;
		case SL_FADE_OUT:
			// TODO
			break;
		case SL_AUTO:
			// TODO
			break;
		case SL_SETUP:
			ESP_LOGE(LOG_TAG, "Exiting to setup mode should not be scheduled");
			break;
	}
}

static void operationTimerCallback(TimerHandle_t timerHandle) {
	LightController::get().executePendingOperation();
}


void LightController::execute(uint8_t* commands, uint32_t byteCount, SmartLightFSM* slFsm) {
	uint32_t cmdSize = sizeof(SmartLightOperation);

	for (uint32_t i = 0; i < byteCount; i += cmdSize) {
		SmartLightOperation operation;
		memcpy(&operation, &commands[i], cmdSize);
		handleOperation(&operation, slFsm);
	}
}

void LightController::handleOperation(SmartLightOperation* operation, SmartLightFSM* slFsm) {
	if (operation->command == SL_SETUP) {
		slFsm->setState(new BleSetupState(slFsm));
		return;
	}

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
			operation,
			operationTimerCallback
		);

		xTimerStart(m_opTimerHandle, 0);
	}
	else {
		performLightOperation(operation, &m_light);
	}
}

void LightController::executePendingOperation() {
	performLightOperation(&m_pendingOperation, &m_light);
}
