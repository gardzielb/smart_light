//
// Created by bartosz on 4/1/22.
//

#ifndef SMART_LIGHT_LIGHTCONTROLLER_H
#define SMART_LIGHT_LIGHTCONTROLLER_H

#include <stdint.h>

#include "main.h"
#include "LedRing.h"

#define SL_COMMAND_MAX_LEN 5

enum SmartLightCommand : uint8_t {
	SL_IDLE = 0x00,
	SL_ON = 0x01,
	SL_OFF = 0x02,
	SL_SET_COLOR = 0x03,
	SL_SET_INTENSITY = 0x04,
	SL_FADE_OUT = 0x05,
	SL_AUTO = 0x06,
	SL_SETUP = 0x07
};

struct SmartLightOperation {
	SmartLightCommand command;
	uint16_t delay;
	uint8_t data[SL_COMMAND_MAX_LEN];
};


class LightController {
public:
	inline LightController()
		: m_light(LED_RING_LED_COUNT, LED_RING_PIN) {}

	inline static LightController& get() {
		static LightController instance;
		return instance;
	}

	inline void initialize() {
		m_light.initialize();
		m_light.setColor(32, 32, 32);
	}

	void execute(uint8_t* commands, uint32_t byteCount);

private:
	void performOperation(SmartLightOperation* operation);

	LedRing m_light;
};


#endif //SMART_LIGHT_LIGHTCONTROLLER_H
