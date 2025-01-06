/*
 * AppConfig.h
 *
 * Created: 12/28/2024 8:19:40 PM
 *  Author: suraj
 */ 
#ifndef APPCONFIG_H_
#define APPCONFIG_H_


/* This is the configuration that will be experienced by the ATMEGA16U2 of this project:
	1. USB protocol
		@ Will be bulk transfer
		@ Will be single endpoint
		@ Will contain timestamp
	2.  */

	#define pin_SS PORTB0
	#define pin_SCK PORTB1
	#define pin_MOSI PORTB2
	#define pin_MISO PORTB3
	#define pin_SPI_PORT PORTB
	#define pin_SPI_DDR DDRB
	#define pin_LED_GPIO PB5
	#define MY_DEVICE_NAME "Schlong 2"
	#define pin_USB_A PORTD0
	#define pin_USB_B PORTD1
	#define pin_USB_C PORTD2
	#define pin_USB_D PORTD3
	#define pin_CLK_SEL_PIN PORTD4
	#define pin_PWR_DWN PIN1
	#define pin_RST PIN2
	#define reg_ADC_register 0x02
	#define cmd_ADC_SDATAC 0b00010001
	#define cmd_ADC_RDATAC 0b00010000
	#define pin_ADC_DRDY 0x03
	#define pin_analogV PD5
	
	/* Function Prototypes */
	void SendSensorData(void);
	void EVENT_USB_Device_ConfigurationChanged(void);

#endif /* APPCONFIG_H_ */