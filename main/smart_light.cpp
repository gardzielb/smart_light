#include <sys/cdefs.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp32-hal-gpio.h>

#include "main.h"
#include "ble/BleSetupState.h"
#include "light/LightController.h"


extern "C" _Noreturn void app_main(void) {
	ESP_LOGI("main", "Starting smart light");

	gpio_set_direction(LED_YELLOW_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_RED_PIN, GPIO_MODE_OUTPUT);

	gpio_set_level(LED_YELLOW_PIN, 0);
	gpio_set_level(LED_GREEN_PIN, 0);
	gpio_set_level(LED_RED_PIN, 0);

	LightController::get().initialize();

	/* Initialize NVS. */
	esp_err_t flash_init_result = nvs_flash_init();
	if (flash_init_result == ESP_ERR_NVS_NO_FREE_PAGES || flash_init_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		flash_init_result = nvs_flash_init();
	}
	ESP_ERROR_CHECK(flash_init_result);

	SmartLightFSM fsm;
//	fsm.setState(new MockLightState(&fsm));
	fsm.setState(new BleSetupState(&fsm));

	while (true) {
		fsm.loop();
	}
}
