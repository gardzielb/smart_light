//
// Created by bartosz on 4/1/22.
//

#include "LightController.h"

void LightController::execute(uint8_t* commands, uint32_t byteCount) {
	uint32_t cmdSize = sizeof(SmartLightOperation);

	for (uint32_t i = 0; i < byteCount; i += cmdSize) {
		SmartLightOperation operation;
		memcpy(&operation, &commands[i], cmdSize);
		performOperation(&operation);
	}
}

void LightController::performOperation(SmartLightOperation* operation) {
	switch (operation->command) {
		case SL_IDLE:
			break;
		case SL_ON:
			m_light.turnOn();
			break;
		case SL_OFF:
			m_light.turnOff();
			break;
		case SL_SET_COLOR:
			m_light.setColor(operation->data[0], operation->data[1], operation->data[2]);
			break;
		case SL_SET_INTENSITY:
			m_light.setIntensity(operation->data[0] / 100.f);
			break;
		case SL_FADE_OUT:
			// TODO
			break;
		case SL_AUTO:
			// TODO
			break;
		case SL_SETUP:
			// TODO
			break;
	}
}
