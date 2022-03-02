//
// Created by bartosz on 2/2/22.
//

#ifndef SMART_LED_COLOR_H
#define SMART_LED_COLOR_H

#include <cstdint>


struct Color {
	uint8_t r, g, b;

	Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

	inline void set(uint8_t r, uint8_t g, uint8_t b) {
		this->r = r;
		this->g = g;
		this->b = b;
	}

	inline Color& operator*=(float a) {
		r *= a;
		g *= a;
		b *= a;
		return *this;
	}
};


inline Color operator*(float a, const Color& c) {
	return Color(a * c.r, a * c.g, a * c.b);
}


#endif //SMART_LED_COLOR_H
