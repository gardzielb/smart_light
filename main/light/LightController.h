//
// Created by bartosz on 4/1/22.
//

#ifndef SMART_LIGHT_LIGHTCONTROLLER_H
#define SMART_LIGHT_LIGHTCONTROLLER_H

#include <stdint.h>

#include "main.h"
#include "LedRing.h"
#include "SmartLightFSM.h"

#define SL_COMMAND_MAX_LEN 64

enum SmartLightCommand : uint8_t {
	SL_IDLE = 0x00,
	SL_ON = 0x01,
	SL_OFF = 0x02,
	SL_SET_COLOR = 0x03,
	SL_SET_INTENSITY = 0x04,
	SL_FADE_OUT = 0x05,
	SL_AUTO = 0x06,
	SL_SETUP = 0x07,
	SL_PING = 0x08,
	SL_JSON = '{'
};

struct __attribute__((__packed__)) SmartLightOperation {
	uint32_t delay;
	uint8_t dataLen;
	uint8_t data[SL_COMMAND_MAX_LEN];
};

struct FadingData {
	LedRing* light;
	TimerHandle_t timerHandle;
	float baseAlpha;
	float alphaStep;
};

struct ExecutionRequest {
	SmartLightFSM* slFsm;
	SmartLightOperation operation;
};


class LightController {
public:
	inline static LightController& get() {
		static LightController instance;
		return instance;
	}

	inline void initialize() {
		m_light.initialize();
		m_light.turnOff();
	}

	inline void setup(Color color, uint8_t intensity) {
		m_light.setColor(color.r, color.g, color.b);
		m_light.setIntensity(intensity / 100);
	}

	void handleOperation(SmartLightOperation* operation, SmartLightFSM* slFsm);

	void executePendingOperation();

	~LightController();

private:
	LightController();

	uint8_t executeCommand(uint8_t cmd, uint8_t* args, SmartLightFSM* slFsm, bool fromIsr);

	void execute(SmartLightOperation* operation, SmartLightFSM* slFsm, bool fromIsr = false);

	void startFading(uint16_t fadingDuration, bool fromIsr);

	LedRing m_light;
	TimerHandle_t m_opTimerHandle = NULL;
	ExecutionRequest m_pendingOperation = {};
	FadingData m_fadingData = {
		.light = &m_light,
		.timerHandle = NULL
	};
};


#endif //SMART_LIGHT_LIGHTCONTROLLER_H
