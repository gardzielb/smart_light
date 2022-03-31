//
// Created by bartosz on 2/2/22.
//

#ifndef SMART_LED_LEDRING_H
#define SMART_LED_LEDRING_H

#include <cstdint>
#include "Adafruit_NeoPixel/Adafruit_NeoPixel.h"

#include "Color.h"


class LedRing {
public:
	LedRing(uint16_t ledCount, gpio_num_t pin, const Color& color = Color(64, 64, 64));

	void initialize();

	void turnOn();

	void turnOff();

	void setColor(uint8_t r, uint8_t g, uint8_t b);

	void setIntensity(float intensity);

private:
	void updateLeds();

	const uint16_t m_ledCount;
	Adafruit_NeoPixel m_leds;

	Color m_color;
	float m_intensity = 1.0f;
	bool m_lightOn = false;
};


#endif //SMART_LED_LEDRING_H
