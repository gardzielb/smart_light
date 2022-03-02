#include <sys/cdefs.h>
#include <esp_log.h>
#include <hal/gpio_types.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "LedRing.h"

#define LED_PIN GPIO_NUM_2
#define LED_RING_PIN GPIO_NUM_5
#define LED_RING_LED_COUNT 12

static LedRing s_ledRing(LED_RING_LED_COUNT, LED_RING_PIN);


extern "C" _Noreturn void app_main(void) {
	ESP_LOGI("main", "Starting smart light");

	s_ledRing.initialize();
	s_ledRing.setColor(64, 0, 64);
	s_ledRing.turnOn();

	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	uint32_t led_state = 1;

	while (true) {
		gpio_set_level(LED_PIN, led_state);
		led_state = 1 - led_state;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
