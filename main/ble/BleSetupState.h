//
// Created by bartosz on 3/3/22.
//

#ifndef SMART_LIGHT_BLESETUPSTATE_H
#define SMART_LIGHT_BLESETUPSTATE_H

#include "SmartLightFSM.h"


class BleSetupState : public SmartLightState {
public:
	BleSetupState(SmartLightFSM* fsm);

	void begin() override;

	void loop() override;
};


enum {
	IDX_SVC,
	IDX_CHAR_A,
	IDX_CHAR_VAL_A,
	IDX_CHAR_CFG_A,

	IDX_CHAR_B,
	IDX_CHAR_VAL_B,

	IDX_CHAR_C,
	IDX_CHAR_VAL_C,

	HRS_IDX_NB,
};


#endif //SMART_LIGHT_BLESETUPSTATE_H
