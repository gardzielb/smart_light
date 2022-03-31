//
// Created by bartosz on 2/2/22.
//

#include "LedRing.h"


LedRing::LedRing(uint16_t ledCount, gpio_num_t pin, const Color& color)
		: m_ledCount(ledCount), m_leds(ledCount, pin, NEO_GRB), m_color(color) {}

void LedRing::initialize() {
	m_leds.begin();
	m_leds.clear();
}

void LedRing::turnOn() {
	updateLeds();
	m_lightOn = true;
}

void LedRing::turnOff() {
	m_leds.clear();
	m_leds.show();
	m_lightOn = false;
}

void LedRing::setColor(uint8_t r, uint8_t g, uint8_t b) {
	m_color.set(r, g, b);
	if (m_lightOn)
		updateLeds();
}

void LedRing::setIntensity(float intensity) {
	m_intensity = intensity;
	if (m_lightOn)
		updateLeds();
}

void LedRing::updateLeds() {
	Color color = m_intensity * m_color;
	for (uint16_t i = 0; i < m_ledCount; i++) {
		m_leds.setPixelColor(i, Adafruit_NeoPixel::Color(color.r, color.g, color.b));
	}
	m_leds.show();
}
