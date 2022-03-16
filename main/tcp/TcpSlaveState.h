//
// Created by bartosz on 3/16/22.
//

#ifndef SMART_LIGHT_TCPSLAVESTATE_H
#define SMART_LIGHT_TCPSLAVESTATE_H

#include "SmartLightFSM.h"


class TcpSlaveState : public SmartLightState {
public:
	TcpSlaveState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;
};


#endif //SMART_LIGHT_TCPSLAVESTATE_H
