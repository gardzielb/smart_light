//
// Created by bartosz on 4/4/22.
//

#include "SetupStorage.h"

#include <esp_log.h>

#define LOG_TAG "SetupStorage"
#define NVS_SETUP_KEY "setup"

SetupStorage::SetupStorage() {
	esp_err_t nvsInitResult = nvs_flash_init();
	if (nvsInitResult == ESP_ERR_NVS_NO_FREE_PAGES || nvsInitResult == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		nvsInitResult = nvs_flash_init();
	}
	ESP_ERROR_CHECK(nvsInitResult);

	esp_err_t nvsOpenResult = nvs_open("storage", NVS_READWRITE, &m_nvsHandle);
	if (nvsOpenResult != ESP_OK) {
		ESP_LOGE(LOG_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(nvsOpenResult));
	}
	else {
		ESP_LOGI(LOG_TAG, "Flash initialized");
	}
}

void SetupStorage::storeSetup(const SetupData* setupData) {
	nvs_set_blob(m_nvsHandle, NVS_SETUP_KEY, setupData, sizeof(SetupData));
}

bool SetupStorage::readSetup(SetupData* outSetupData) {
	size_t dataLen;
	esp_err_t nvsReadResult = nvs_get_blob(m_nvsHandle, NVS_SETUP_KEY, outSetupData, &dataLen);
	return nvsReadResult == ESP_OK;
}

void SetupStorage::storeModeSetup(SmartLightMode mode, const void* data, size_t dataLen) {
	char key[15];
	sprintf(key, "mode_%u_data", mode);
	nvs_set_blob(m_nvsHandle, key, data, dataLen);
}

bool SetupStorage::readModeSetup(SmartLightMode mode, void* outData, size_t dataLen) {
	char key[15];
	sprintf(key, "mode_%u_data", mode);
	esp_err_t nvsReadResult = nvs_get_blob(m_nvsHandle, key, outData, &dataLen);

	if (nvsReadResult != ESP_OK) {
		ESP_LOGW(LOG_TAG, "Failed to read %u mode config from flash: %s", mode, esp_err_to_name(nvsReadResult));
	}

	return nvsReadResult == ESP_OK;
}
