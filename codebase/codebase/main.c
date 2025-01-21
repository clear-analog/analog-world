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
 *
 *  Main source file for the Audio Input demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "Config/AppConfig.h"
#include <stdlib.h>

/* Globals for CDC state (whatever this means)
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
	.Config = {
		.ControlInterfaceNumber = INTERFACE_ID_SerMon_Ctrl,
		.DataINEndpoint = {
			// We are using existing definitions from Descriptors.h
			.Address = CDC_TX_EPADDR,
			
			.Size = CDC_TXRX_EPSIZE,
			
			.Banks = 1,
		},
		.DataOUTEndpoint = {
			.Address = CDC_RX_EPADDR,
			
			.Size = CDC_TXRX_EPSIZE,
			
			.Banks = 1,
		},
		.NotificationEndpoint = {
			.Address = CDC_NOTIFICATION_EPADDR,
			
			.Size = CDC_NOTIFICATION_EPSIZE,
			
			.Banks = 1,
		}
	}
};*/

/** Flag to indicate if the streaming audio alternative interface has been selected by the host. */
static bool StreamingAudioInterfaceSelected = false;

/** Current audio sampling frequency of the streaming audio endpoint. */
static uint32_t CurrentAudioSampleFrequency = 48000;

/*/ Serial debug functions that send data to Python host
void SerialDebug(uint8_t debug_code, uint8_t value) {
	if (USB_DeviceState != DEVICE_STATE_Configured) return;
	
	// Send debug packet format: [0xFF, debug_code, value]
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, 0xFF); // Start marker
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, debug_code);
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, value);
}*/

// Send a null-terminated string to serial monitor
void SerialSendString(const char* str) {
	if (USB_DeviceState != DEVICE_STATE_Configured) return;
	
	while (*str) {
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, *str++);
	}
	
	// Optional: Send newline characters
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\r');
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\n');
}

// Debug codes
#define DEBUG_SPI_RECEIVED 0x01
#define DEBUG_USB_STATUS   0x02
#define DEBUG_TIMER_EVENT  0x03
#define DEBUG_ERROR        0xFF

// This function will setup a timer to be used. Read through all the important bits and pieces of this puzzle
// What I need to learn is exactly how
void setupTimer0(void) {
	TCCR0A = (1 << WGM01); // reading up on all possible values of this will be important
	TCCR0B = (1 << CS01) | (1 << CS00); // Same here
	OCR0A = 249;
	TCNT0 = 0;
}

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void) {
	//ddr_GPIO |= (1 << pin_LED_DEBUG);
	//port_GPIO |= (1 << pin_LED_DEBUG);
	/* Blink Debug LED in startup sequence
	lightUp(1, pin_LED_DEBUG, 1000);
	lightUp(2, pin_LED_DEBUG, 1000);
	lightUp(3, pin_LED_DEBUG, 1000);
	// Set SPI LED high and low in startup sequence as well
	lightUp(1, pin_SS, 1000);
	lightUp(2, pin_SS, 1000);
	lightUp(3, pin_SS, 1000);*/
				
	SetupHardware();

	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	
	//ddr_SPI &= ~(1 << pin_SS);
	//delay_sck_cycles(2^20);
	//ddr_SPI |= (1 << pin_SS);
/* 	#ifdef TEST_ALL
		#define TEST_LED
		#define TEST_SPI
		#define TEST_ADC
		#define TEST_USB
	#endif

	#ifdef TEST_LED
		#include "Tests/test_led.c"
		test_led();
	#endif

	#ifdef TEST_SPI
		#include "Tests/test_spi.c"
		test_spi();
	#endif

	#ifdef TEST_ADC
		#include "Tests/test_adc.c"
		test_adc();
	#endif

	#ifdef TEST_USB
		#include "Tests/test_USB.c"
		test_usb();
	#endif */	

	long long int a = 0;
	/*uint8_t values[1] = {5};
	uint8_t add = 0x07; //address of register being changed
	uint8_t prev_val[1];*/

	for (;;) {
		USB_USBTask();
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		//port_GPIO &= ~(1 << pin_LED_DEBUG);
		//delay_sck_cycles(time2sck(1500));
		port_GPIO |= (1 << pin_LED_DEBUG);
		
		//SerialSendString("Setup done");

	/*
		if (a > 100000000) {
			ADS1299_RREG(add, prev_val,1 );
			ADS1299_WREG(add, values, 1);
			ADS1299_RREG(add, values, 1);
			lightUp(prev_val[0], pin_LED_DEBUG, 1000.0f);
			ADS1299_WREG(add, prev_val, 1);
			a -= 100000000;
		}
		a++;
		port_GPIO &= ~(1 << pin_LED_DEBUG);
	*/
	
		if (a > 100000) {
			//SerialSendString("Setup Done");
			if (USB_DeviceState == DEVICE_STATE_Configured) {
				PORTC |= (1 << PORTC7);  // LED on
 
				CDC_Device_SendString(&VirtualSerial_CDC_Interface, "Debug message\r\n");
			}
			a -= 100000;
		}
		
		a++;
	}
}

// Configures the board hardware and chip peripherals.
void SetupHardware(void) {
	#if (ARCH == ARCH_AVR8)
		// Disable watchdog if enabled by bootloader/fuses
		MCUSR &= ~(1 << WDRF);
		wdt_disable();

		// Disable clock division
		clock_prescale_set(clock_div_1);
	#endif
	
	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization
	//LEDs_Init();
	//Buttons_Init();
	//ADC_Init(ADC_FREE_RUNNING | ADC_PRESCALE_32);
	//ADC_SetupChannel(MIC_IN_ADC_CHANNEL);

	//Start the ADC conversion in free running mode
	//ADC_StartReading(ADC_REFERENCE_AVCC | ADC_RIGHT_ADJUSTED | ADC_GET_CHANNEL_MASK(MIC_IN_ADC_CHANNEL));
	_ADS1299_MODE = ADS1299_MODE_WAKEUP;

	// SPI Configuration (ddr_SPI = DDRB)
	ddr_SPI |= (1 << pin_MOSI) | (1 << pin_SCK) | (1 << pin_SS); //MOSI, SCK, SS are outputs
	ddr_SPI &= ~(1 << pin_MISO);
	ddr_GPIO &= ~(1 << pin_ADC_DRDY); // Set pin_ADC_DRDY as input

	// Setting Clock rate to fck/16 and enabling SPI as master.
	SPCR = (1 << SPE) | (1 << MSTR) | (1<<SPI2X) | (1 << SPR0) | (0 << CPOL) | (0 << CPHA);
	ADS1299_SETUP();
	
	delay_sck_cycles(2^15);*/

	USB_Init();
	sei();
	// Setup onboard LED as output
	DDRC |= (1 << PC7);

	// Set LED based on USB configuration state
	if (USB_DeviceState == DEVICE_STATE_Configured)
		PORTC |= (1 << PC7);  // LED on
	else 
		PORTC &= ~(1 << PC7); // LED off
	CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	//static FILE USBSerialStream;
	//CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
	//CDC_Device_CreateStream()
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs, and
 *  configures the sample update and PWM timers.
 */
void EVENT_USB_Device_Connect(void) {
	/* Indicate USB enumerating */
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	/* Sample reload timer initialization */
	TIMSK0  = (1 << OCIE0A);
	OCR0A   = ((F_CPU / 8 / CurrentAudioSampleFrequency) - 1);
	TCCR0A  = (1 << WGM01);  // CTC mode
	TCCR0B  = (1 << CS01);   // Fcpu/8 speed
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs, disables the sample update and PWM output timers and stops the USB and Audio management tasks.
 */
void EVENT_USB_Device_Disconnect(void) {
	/* Stop the sample reload timer */
	TCCR0B = 0;

	/* Indicate streaming audio interface not selected */
	StreamingAudioInterfaceSelected = false;

	/* Indicate USB not ready */
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host set the current configuration
 *  of the USB device after enumeration - the device endpoints are configured.
 */
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	/* Setup Audio Stream Endpoint */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(AUDIO_STREAM_EPADDR, EP_TYPE_ISOCHRONOUS, AUDIO_STREAM_EPSIZE, 2);

	/* Indicate endpoint configuration success or failure */
	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void) {
	/* Process General and Audio specific control requests */
	switch (USB_ControlRequest.bRequest) {
		case REQ_SetInterface:
			/* Set Interface is not handled by the library, as its function is application-specific */
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_STANDARD | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				/* Check if the host is enabling the audio interface (setting AlternateSetting to 1) */
				StreamingAudioInterfaceSelected = ((USB_ControlRequest.wValue) != 0);
			}

			break;
		case AUDIO_REQ_GetStatus:
			/* Get Status request can be directed at either the interface or endpoint, neither is currently used
			 * according to the latest USB Audio 1.0 standard, but must be ACKed with no data when requested */
			if ((USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) ||
			    (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_ENDPOINT)))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();
			}

			break;
		case AUDIO_REQ_SetCurrent:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_ENDPOINT))
			{
				/* Extract out the relevant request information to get the target Endpoint address and control being set */
				uint8_t EndpointAddress = (uint8_t)USB_ControlRequest.wIndex;
				uint8_t EndpointControl = (USB_ControlRequest.wValue >> 8);

				/* Only handle SET CURRENT requests to the audio endpoint's sample frequency property */
				if ((EndpointAddress == AUDIO_STREAM_EPADDR) && (EndpointControl == AUDIO_EPCONTROL_SamplingFreq))
				{
					uint8_t SampleRate[3];

					Endpoint_ClearSETUP();
					Endpoint_Read_Control_Stream_LE(SampleRate, sizeof(SampleRate));
					Endpoint_ClearIN();

					/* Set the new sampling frequency to the value given by the host */
					CurrentAudioSampleFrequency = (((uint32_t)SampleRate[2] << 16) | ((uint32_t)SampleRate[1] << 8) | (uint32_t)SampleRate[0]);

					/* Adjust sample reload timer to the new frequency */
					OCR0A = ((F_CPU / 8 / CurrentAudioSampleFrequency) - 1);
				}
			}

			break;
		case AUDIO_REQ_GetCurrent:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_ENDPOINT))
			{
				/* Extract out the relevant request information to get the target Endpoint address and control being retrieved */
				uint8_t EndpointAddress = (uint8_t)USB_ControlRequest.wIndex;
				uint8_t EndpointControl = (USB_ControlRequest.wValue >> 8);

				/* Only handle GET CURRENT requests to the audio endpoint's sample frequency property */
				if ((EndpointAddress == AUDIO_STREAM_EPADDR) && (EndpointControl == AUDIO_EPCONTROL_SamplingFreq))
				{
					uint8_t SampleRate[3];

					/* Convert the sampling rate value into the 24-bit format the host expects for the property */
					SampleRate[2] = (CurrentAudioSampleFrequency >> 16);
					SampleRate[1] = (CurrentAudioSampleFrequency >> 8);
					SampleRate[0] = (CurrentAudioSampleFrequency &  0xFF);

					Endpoint_ClearSETUP();
					Endpoint_Write_Control_Stream_LE(SampleRate, sizeof(SampleRate));
					Endpoint_ClearOUT();
				}
			}

			break;
	}
}

// ISR to handle the reloading of the data endpoint with the next sample.
ISR(TIMER0_COMPA_vect, ISR_BLOCK) {
	uint8_t PrevEndpoint = Endpoint_GetCurrentEndpoint();

	/* Select the audio stream endpoint */
	Endpoint_SelectEndpoint(AUDIO_STREAM_EPADDR);

	/* Check if the current endpoint can be written to and that the audio interface is enabled */
	if (Endpoint_IsINReady() && StreamingAudioInterfaceSelected)
	{
		int16_t AudioSample;

		/* Generate random sample between -32768 and 32767 */
		AudioSample = (rand() % 65536) - 32768;

		/* Write the sample to the buffer */
		Endpoint_Write_16_LE(AudioSample);

		/* Check to see if the bank is now full */
		if (!(Endpoint_IsReadWriteAllowed()))
		{
			/* Send the full packet to the host */
			Endpoint_ClearIN();
		}
	}

	Endpoint_SelectEndpoint(PrevEndpoint);
}

// Use this to receive more than one byte
uint8_t SPI_ReceiveByte(void) {
	SPDR = 0;
	while(!(SPSR & (1<<SPIF)));
	uint8_t rcv = SPDR;
	return rcv;
}

// Send a single byte via SPI and wait for transmission to complete. If cont = true, wont mess with SS pin at all
void SPI_SendByte(uint8_t byte, bool cont) {
	if (!cont) {
		SET_SPI_SS(false);
		SCK_ctrl(true);
		SPDR = byte;                      // Start transmission by writing to SPDR
		while(!(SPSR & (1<<SPIF)));       // Wait for transmission to complete
		SET_SPI_SS(true);
		SCK_ctrl(false);
	}

	else {
		SPDR = byte;
		while(!(SPSR & (1 << SPIF)));
	}
}

// This function will write numReg registers starting at regAdd, obtaining necessary values from values byte array
// This function can work in any mode
void ADS1299_WREG(uint8_t regAdd, uint8_t* values, uint8_t numRegs) {
	// Wait for DRDY to go low
	while(port_GPIO & (1 << pin_ADC_DRDY));
	
	
	// Delay for 4 SCK cycles
	delay_sck_cycles(4);
	
	SET_SPI_SS(false); // Set SS low
	SCK_ctrl(true);
	
	// Send first byte: 001r rrrr where rrrrr is register address
	SPI_SendByte(0x40 | (regAdd & 0x1F), true); // Send byte in continous mode
	
	// Send second byte: 000n nnnn where nnnnn is number of registers - 1
	SPI_SendByte(numRegs - 1, true);
	
	// Send the values for each register
	for(uint8_t i = 0; i < numRegs; i++) {
		SPI_SendByte(values[i], true);
	}
	
	SET_SPI_SS(true); // Set SS high
	SCK_ctrl(false);
}

// This function will read numReg registers starting at regAdd and store the data in the provided buffer
// If ADS1299 is in RDATAC mode, SDATAC cmd must be sent first
void ADS1299_RREG(uint8_t regAdd, uint8_t* buffer, uint8_t numRegs) {
    // If in RDATAC mode, must send SDATAC first
    if (_ADS1299_MODE == ADS1299_MODE_RDATAC) {
        ADS1299_SDATAC();
    }

    // Wait for DRDY to go low
    while(port_GPIO & (1 << pin_ADC_DRDY));
    
    // Delay for 4 SCK cycles
    delay_sck_cycles(4);

    SET_SPI_SS(false); // Set SS low
	SCK_ctrl(true);
    
    // Send first byte: 001r rrrr where rrrrr is register address
    SPI_SendByte(0x20 | (regAdd & 0x1F), true); // Continous mode to not affect SS pin throughout CMD
    
    // Send second byte: 000n nnnn where nnnnn is number of registers - 1
    SPI_SendByte(numRegs - 1, true);
    
    // For reading registers, we need to send dummy bytes (0x00) to generate
    // the clock signal for the ADS1299 to send data back
    for(uint8_t i = 0; i < numRegs; i++) {
        SPDR = 0x00; // Send dummy byte to generate clock
        while(!(SPSR & (1<<SPIF))); // Wait for transfer to complete
        buffer[i] = SPDR; // Store the received data in the buffer
    }
    
    SET_SPI_SS(true); // Set SS high
	SCK_ctrl(false);
}

// This function is to send the SDATAC cmd to ADS1299
void ADS1299_SDATAC(void) {
	// Wait for DRDY to go low
	while(port_GPIO & (1 << pin_ADC_DRDY));
	
	// Delay for 4 SCK cycles
	delay_sck_cycles(4);
	
	SPI_SendByte(CMD_ADC_SDATAC, false);
	_ADS1299_MODE = ADS1299_MODE_SDATAC;
}

// This function is to send RDATAC cmd to ADS1299
void ADS1299_RDATAC(void) {
	// Wait for DRDY to go low
	while(port_GPIO & (1 << pin_ADC_DRDY));
	
	// Delay for 4 SCK cycles
	delay_sck_cycles(4);
	
	SPI_SendByte(CMD_ADC_RDATAC, false);
	_ADS1299_MODE = ADS1299_MODE_RDATAC;
}

// This function handles LED debugging based on device mode
// Time in milliseconds
// Number of cycles here is based on ATMEGA16U2 clock
bool lightUp(uint8_t num, uint8_t GPIO_pin, float time) {
	uint32_t cycles = (uint32_t)(F_CPU * (time/1000) / num); // Convert time into cycles, divide by number of times to blink in a second
	for (int i = 0; i < num; i++) {
		port_GPIO |= (1 << GPIO_pin);
		delay_sck_cycles(cycles/2);
		port_GPIO &= ~(1 << GPIO_pin);
		delay_sck_cycles(cycles/2);
	}

	return true;
}

// This function converts time into SCK cycles assuming 4 MHz clock freq
// Time in milliseconds
uint32_t time2sck(float time) {
    // Convert milliseconds to seconds
    float seconds = time / 1000.0f;
    
    // Calculate number of clock cycles
    // Clock frequency is 4MHz = 4,000,000 cycles/second
    uint32_t cycles = (uint32_t)(4000000.0f * seconds);
    
    return cycles;
}

// This function handles delays with ISR
// Used mainly for data processing, not LED debugging
// Tied to SCK clock which is at 4 MHz. This is also F_CPU / 4
void delay_sck_cycles(uint32_t sck_cycles) {
	uint32_t timer_ticks = sck_cycles * 4;
	TCCR1A = 0;
	TCCR1B = (1 << CS10);

	TCNT1 = 0;
	while (TCNT1 < timer_ticks) {
		if (TCNT1 > 65000 && timer_ticks > 65000) { // Overflow handling
			timer_ticks -= 65000;
			TCNT1 = 0;
		}
	}

	TCCR1B = 0;
}

// This function supposedly starts and stops SCK signal as well
void SCK_ctrl(bool input) {
	if (input) {
		SPCR |= (1 << SPE);
	}

	else {
		SPCR &= ~(1 << SPE);
		port_SPI &= ~(1 << pin_SCK);
	}
}

// This function sets up the ADS1299
void ADS1299_SETUP(void) {
	SET_CLK_SEL(true);
	delay_sck_cycles(2^20);  // Changed 2^20 to 1 << 20 for proper bit shifting
	SET_PWR_DWN(false);
	SET_RST(false);
	delay_sck_cycles(2^20);
	SET_PWR_DWN(true);
	SET_RST(true);
	delay_sck_cycles(2^20);
	ADS1299_SDATAC();
	uint8_t refbuf[] = {0b11100000};
	ADS1299_WREG(0x3, refbuf, 1);
	delay_sck_cycles(2^ 20);
	
	SET_SPI_SS(false);
	delay_sck_cycles(2^21);
	SET_SPI_SS(true);
	delay_sck_cycles(2^21);
	SET_SPI_SS(false);

	uint8_t i = 0;
	while (i < size_reg_ls) {
		const regVal_pair temp = ADS1299_REGISTER_LS[i];  // Initialize struct directly
		if (temp.add == -2) {  // Use dot notation instead of arrow operator
			i++;
			continue;
		}
		uint8_t value[] = {(uint8_t)temp.reg_val};  // Use dot notation
		ADS1299_WREG(temp.add, value, 1);  // Use dot notation
		delay_sck_cycles(10);
		i++;
	}
}