#include <sys/cdefs.h>
#include <esp_log.h>
#include <esp32-hal-gpio.h>

#include "main.h"
#include "setup/BleSetupState.h"
#include "light/LightController.h"
#include "setup/SetupStorage.h"
#include "mqtt/MqttSlaveState.h"
#include "tcp/TcpSlaveState.h"
#include "wifi/WiFi.h"

#define LOG_TAG "main"


void sayHello() {
	gpio_set_level(LED_YELLOW_PIN, 0);
	gpio_set_level(LED_GREEN_PIN, 0);
	gpio_set_level(LED_RED_PIN, 0);

	gpio_set_level(LED_YELLOW_PIN, 1);
	vTaskDelay(250 / portTICK_PERIOD_MS);

	gpio_set_level(LED_GREEN_PIN, 1);
	vTaskDelay(250 / portTICK_PERIOD_MS);

	gpio_set_level(LED_RED_PIN, 1);
	vTaskDelay(500 / portTICK_PERIOD_MS);

	gpio_set_level(LED_YELLOW_PIN, 0);
	gpio_set_level(LED_GREEN_PIN, 0);
	gpio_set_level(LED_RED_PIN, 0);
	vTaskDelay(500 / portTICK_PERIOD_MS);

	gpio_set_level(LED_YELLOW_PIN, 1);
	gpio_set_level(LED_GREEN_PIN, 1);
	gpio_set_level(LED_RED_PIN, 1);
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	gpio_set_level(LED_YELLOW_PIN, 0);
	gpio_set_level(LED_GREEN_PIN, 0);
	gpio_set_level(LED_RED_PIN, 0);
}


extern "C" _Noreturn void app_main(void) {
	ESP_LOGI(LOG_TAG, "Starting smart light");
	uint8_t bleMac[6];
	if (esp_read_mac(bleMac, ESP_MAC_BT) == ESP_OK) {
		ESP_LOGI(
			LOG_TAG, "BLE MAC: %02X:%02X:%02X:%02X:%02X:%02X",
			bleMac[0], bleMac[1], bleMac[2], bleMac[3], bleMac[4], bleMac[5]
		);
	}
	else {
		ESP_LOGE(LOG_TAG, "Failed to read BLE mac address");
	}

	gpio_set_direction(LED_YELLOW_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_RED_PIN, GPIO_MODE_OUTPUT);

	sayHello();

	auto& storage = SetupStorage::get();
	auto& lightController = LightController::get();
	lightController.initialize();

	SmartLightFSM fsm;
	SetupData setupData;
	bool bleSetupNecessary = true;

	if (storage.readSetup(&setupData)) {
		if (setupData.mode == SM_MODE_MQTT) {
			wifiInit();
			wifiConnect(setupData.wifiSsid, setupData.wifiPasswd);

			fsm.setState(new TcpSlaveState(&fsm));
			bleSetupNecessary = false;

			MqttConfig mqttConfig;
			if (storage.readModeSetup((SmartLightMode) setupData.mode, &mqttConfig, sizeof(mqttConfig))) {
				ESP_LOGI("main", "I'm %s", mqttConfig.deviceName);
				fsm.setState(new MqttSlaveState(&fsm, mqttConfig));
				bleSetupNecessary = false;
			}
		}
	}

	if (bleSetupNecessary) {
		fsm.setState(new BleSetupState(&fsm));
	}

	while (true) {
		fsm.loop();
	}
}
