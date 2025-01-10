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
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/interrupt.h>

	#include "../Descriptors.h"
	#include <LUFA/Drivers/USB/USB.h>
	#include <LUFA/Drivers/Board/LEDs.h>
	#include <LUFA/Drivers/Board/Buttons.h>
	#include <LUFA/Drivers/Peripheral/ADC.h>
	#include <LUFA/Platform/Platform.h>
	
	/* SPI Definitions */
	#define pin_SS 				PORTB0 // NO NEED FOR BIT MANIPULATION WITH THESE GUYS
	#define pin_SCK 			PORTB1
	#define pin_MOSI 			PORTB2
	#define pin_MISO 			PORTB3
	#define pin_SPI_PORT 		PORTB
	#define pin_SPI_DDR 		DDRB
	#define pin_LED_GPIO 		PORTB6
	#define MY_DEVICE_NAME		"Schlong Kingdom 2"
	#define MIC_IN_ADC_CHANNEL	2
	#define pin_CLK_SEL_PIN 	PORTD4
	#define pin_PWR_DWN 		PIN1
	#define pin_RST 			PIN2
	#define reg_ADC_register 	0x02
	#define cmd_ADC_SDATAC 		0b00010001
	#define cmd_ADC_RDATAC 		0b00010000
	#define pin_ADC_DRDY 		0x03
	
	// To Be Deleted
	#define pin_USB_A PORTD0
	#define pin_USB_B PORTD1
	#define pin_USB_C PORTD2
	#define pin_USB_D PORTD3	

	/* Macros: */
	/** Maximum audio sample value for the microphone input. */
	#define SAMPLE_MAX_RANGE          0xFFFF
	/** Maximum ADC range for the microphone input. */
	#define ADC_MAX_RANGE             0x3FF
	/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
	#define LEDMASK_USB_NOTREADY      LEDS_LED1
	/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
	#define LEDMASK_USB_ENUMERATING  (LEDS_LED2 | LEDS_LED3)
	/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
	#define LEDMASK_USB_READY        (LEDS_LED2 | LEDS_LED4)
	/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
	#define LEDMASK_USB_ERROR        (LEDS_LED1 | LEDS_LED3)

	/* Function Prototypes: */
	void SetupHardware(void); // This function must initialize the 
	void EVENT_USB_Device_Connect(void);
	void EVENT_USB_Device_Disconnect(void);
	void EVENT_USB_Device_ConfigurationChanged(void);
	void EVENT_USB_Device_ControlRequest(void);
	void ADS1299_REG_WR(uint8_t, uint8_t);
	void ADS1299_REG_RD(uint8_t);
	void ADS1299_SETUP(void);
	void ADS1299_SDATAC(void);
	void ADS1299_PWR_UP(void);
	void ADS1299_RDATAC(void);
	void ADS1299_TOM(void);
	void SPI_send();
	void SendSensorData(void);

	#define MICROPHONE_BIASED_TO_HALF_RAIL
	#define USE_TEST_TONE

#endif
