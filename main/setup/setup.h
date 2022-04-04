//
// Created by bartosz on 4/4/22.
//

#ifndef SMART_LIGHT_SETUP_H
#define SMART_LIGHT_SETUP_H

#include <stdint.h>

#define WIFI_CRED_MAX_LEN 20

enum SmartLightMode : uint8_t {
	SM_MODE_MQTT,
	SM_MODE_TCP
};


enum SetupStatus {
	SETUP_IN_PROGRESS,
	SETUP_READY,
	SETUP_WIFI_CONNECTING,
	SETUP_WIFI_CONNECTED,
	SETUP_DONE
};


struct SetupData {
	char wifiSsid[20];
	char wifiPasswd[20];
	uint8_t mode;
	uint8_t controlPoint;
};

#endif //SMART_LIGHT_SETUP_H
