/*
 * test_led.c
 *
 * Created: 1/14/2025 2:25:06 PM
 *  Author: suraj
 */ 

#include "../Config/AppConfig.h"

int test_led(void) {
	// Blink Debug LED in startup sequence
	lightUp(1, pin_LED_DEBUG, 1000);
	lightUp(2, pin_LED_DEBUG, 1000);
	lightUp(3, pin_LED_DEBUG, 1000);

	// Set SPI LED high and low in startup sequence as well
	lightUp(1, pin_SS, 1000);
	lightUp(2, pin_SS, 1000);
	lightUp(3, pin_SS, 1000);

	return 0;
}