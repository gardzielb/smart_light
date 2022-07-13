//
// Created by bartosz on 3/3/22.
//

#ifndef SMART_LIGHT_BLINKSTATE_H
#define SMART_LIGHT_BLINKSTATE_H

#include "SmartLightFSM.h"
#include "LedRing.h"


class BlinkState : public SmartLightState {
public:
	BlinkState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;

private:
	uint32_t m_statusLedState = 0;
};


#endif //SMART_LIGHT_BLINKSTATE_H
