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

	IDX_CHAR_WIFI_SSID,
	IDX_CHAR_VAL_WIFI_SSID,

	IDX_CHAR_WIFI_PASSWD,
	IDX_CHAR_VAL_WIFI_PASSWD,

	IDX_CHAR_MODE,
	IDX_CHAR_VAL_MODE,

	IDX_CHAR_CONTROL_POINT,
	IDX_CHAR_VAL_CONTROL_POINT,
	IDX_CHAR_CFG_CONTROL_POINT,

	HRS_IDX_COUNT,
};


#endif //SMART_LIGHT_BLESETUPSTATE_H
