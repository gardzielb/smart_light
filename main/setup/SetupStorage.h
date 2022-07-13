//
// Created by bartosz on 4/4/22.
//

#ifndef SMART_LIGHT_SETUPSTORAGE_H
#define SMART_LIGHT_SETUPSTORAGE_H

#include <nvs_flash.h>

#include "setup.h"

class SetupStorage {
public:
	inline static SetupStorage& get() {
		static SetupStorage instance;
		return instance;
	}

	void storeSetup(const SetupData* setupData);

	bool readSetup(SetupData* outSetupData);

	void storeModeSetup(SmartLightMode mode, const void* data, size_t dataLen);

	bool readModeSetup(SmartLightMode mode, void* outData, size_t dataLen);

	void clear();

private:
	SetupStorage();

	nvs_handle_t m_nvsHandle;
};


#endif //SMART_LIGHT_SETUPSTORAGE_H
