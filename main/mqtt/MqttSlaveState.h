//
// Created by bartosz on 3/16/22.
//

#ifndef SMART_LIGHT_MQTTSLAVESTATE_H
#define SMART_LIGHT_MQTTSLAVESTATE_H

#include "SmartLightFSM.h"


class MqttSlaveState : public SmartLightState {
public:
	MqttSlaveState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;
};


#endif //SMART_LIGHT_MQTTSLAVESTATE_H
