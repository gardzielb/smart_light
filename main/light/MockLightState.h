//
// Created by bartosz on 3/3/22.
//

#ifndef SMART_LIGHT_MOCKLIGHTSTATE_H
#define SMART_LIGHT_MOCKLIGHTSTATE_H

#include "SmartLightFSM.h"
#include "LedRing.h"


class MockLightState : public SmartLightState {
public:
	MockLightState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;

private:
	LedRing m_ledRing;
	uint32_t m_statusLedState = 0;
};


#endif //SMART_LIGHT_MOCKLIGHTSTATE_H
