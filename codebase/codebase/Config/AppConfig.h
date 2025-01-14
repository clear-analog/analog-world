/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *  \brief Application Configuration Header File
 *
 *  This is a header file which is be used to configure some of
 *  the application's compile time options, as an alternative to
 *  specifying the compile time constants supplied through a
 *  makefile or build system.
 *
 *  For information on what each token does, refer to the
 *  \ref Sec_Options section of the application documentation.
 */

#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_	
	// Deez Includes
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/interrupt.h>
	#include "../Descriptors.h"
	#include <LUFA/Drivers/USB/USB.h>
	#include <LUFA/Platform/Platform.h>
	#include <LUFA/Drivers/Board/LEDs.h>
	#include <LUFA/Drivers/Board/Buttons.h>
	#include <LUFA/Drivers/Peripheral/ADC.h>
	
	/* Definitions */
	#define pin_SS 				PORTB0
	#define pin_SCK 			PORTB1
	#define pin_MOSI 			PORTB2
	#define pin_MISO 			PORTB3
	#define ddr_SPI 			DDRB
	#define port_SPI 			PORTB
	#define port_LED			PORTB
	#define pin_LED_DEBUG 		PORTB6
	#define pin_CLK_SEL			PORTD4
	#define pin_PWR_DWN 		PIN1
	#define pin_RST 			PIN2
	#define reg_ADC_register 	0x02
	#define pin_ADC_DRDY 		0x03

	// ADC Register Settings
	#define CMD_ADC_SDATAC 		0b00010001
	#define CMD_ADC_RDATAC 		0b00010000
	struct Deez {
		int add;
		int reg_val;
	} Nutz;

	// Registers to setup. If -2, end WREG.
	struct Nutz ADS1299_REGISTER_LS[] = {
		{0x01, 0b10110000},
		{0x02, 0b11010000},
		{0x03, 0b11101100},
		{0x04, 0},
		{-2, -2},
		{0x05, 0b01100000},
		{0x06, 0b01100000},
		{0x07, 0b01100000},
		{0x08, 0b01100000},
		{0x09, 0b01100000},
		{0x0A, 0b01100000},
		{0x0B, 0b01100000},
		{0x0C, 0b01100000},
		{0x0D, 0b11111111},
		{0x0E, 0b11111111},
		{0x0F, 0},
		{0x10, 0},
		{0x11, 0},
		{-2, -2},
		{0x15, 0},
		{0x16, 0},
		{0x17, 0}
	};

	bool lightUp(uint8_t num, uint8_t pin, uint16_t time); // Board Debugging Function

	/* USB Functions */
	void EVENT_USB_Device_Connect(void);
	void EVENT_USB_Device_Disconnect(void);
	void EVENT_USB_Device_ConfigurationChanged(void);
	void EVENT_USB_Device_ControlRequest(void);
	void SetupHardware(void); // This function configures {ATMEGA: SPI & USB}, {ADC: SPI and Registers}

	// ADS1299 operation modes
	#define ADS1299_MODE_SDATAC 0
	#define ADS1299_MODE_RDATAC 1
	#define ADS1299_MODE_WAKEUP 10
	uint8_t _ADS1299_MODE;

	// Encapsulations of ADS1299 commands
	void ADS1299_WREG(uint8_t, uint8_t*, uint8_t);
	void ADS1299_RREG(uint8_t, uint8_t*, uint8_t);
	void ADS1299_SETUP(void);
	void ADS1299_SDATAC(void);
	void ADS1299_RDATAC(void);

	// SPI Core Functions
	void SPI_SendByte(uint8_t byte, bool cont);
	void delay_sck_cycles(uint32_t);
	static inline void SET_CLK_SEL(const bool input) __attribute__((always_inline));
	static inline void SET_PWR_DWN(const bool input) __attribute__((always_inline));
	static inline void SET_SPI_SS(const bool input) __attribute__((always_inline));
	static inline void SET_RST(const bool input) __attribute__((always_inline));

	/* Unnecessary Things From LUFA */
	#define SAMPLE_MAX_RANGE          0xFFFF //Maximum audio sample value for the microphone input.
	#define ADC_MAX_RANGE             0x3FF //Maximum ADC range for the microphone input.
	#define MY_DEVICE_NAME		      "Schlong Kingdom 2"
	#define LEDMASK_USB_NOTREADY      LEDS_LED1 //LED mask for the library LED driver, to indicate that the USB interface is not ready.
	#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3) //LED mask for the library LED driver, to indicate that the USB interface is enumerating.
	#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4) //LED mask for the library LED driver, to indicate that the USB interface is ready.
	#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3) //LED mask for the library LED driver, to indicate that an error has occurred in the USB interface.
	#define MICROPHONE_BIASED_TO_HALF_RAIL
	#define USE_TEST_TONE
	#define MIC_IN_ADC_CHANNEL	2
#endif