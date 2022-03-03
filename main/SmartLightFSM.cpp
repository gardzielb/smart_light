//
// Created by bartosz on 3/3/22.
//

#include "SmartLightFSM.h"


void SmartLightFSM::loop() {
	if (m_state)
		m_state->loop();
}

void SmartLightFSM::setState(SmartLightState* state) {
	if (m_state)
		delete m_state;
	m_state = state;
	m_state->begin();
}

SmartLightFSM::~SmartLightFSM() {
	if (m_state)
		delete m_state;
}
