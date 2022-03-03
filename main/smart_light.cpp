#include <sys/cdefs.h>
#include <esp_log.h>

#include "MockLightState.h"


extern "C" _Noreturn void app_main(void) {
	ESP_LOGI("main", "Starting smart light");

	SmartLightFSM fsm;
	fsm.setState(new MockLightState(&fsm));

	while (true) {
		fsm.loop();
	}
}
