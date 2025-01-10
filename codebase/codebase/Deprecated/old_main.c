/*
 * codebase.c
 *
 * Created: 12/12/2024 4:04:31 PM
 * Author : suraj
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> // Added for rand()
#include "Descriptors.h"
#include "Config/AppConfig.h"

// USB configuration globals
static uint32_t timestamp_counter = 0;
static volatile uint8_t usb_configuration = 0;
static uint8_t endpoint_buffer[BULK_IN_EPSIZE]; //CERELOG CHANGE THIS
static volatile bool transfer_complete = true;

// Circular buffer for SPI to USB transfer
#define BUFFER_SIZE 256
static volatile uint8_t circular_buffer[BUFFER_SIZE];
static volatile uint8_t buffer_head = 0;
static volatile uint8_t buffer_tail = 0;

// Probe voltage data packet
typedef struct {
    uint32_t timestamp;
    uint16_t probe_values[8];
} voltage_packet_t;

// USB device state machine states
typedef enum {
    DEVICE_STATE_UNATTACHED,
    DEVICE_STATE_ATTACHED,
    DEVICE_STATE_POWERED,
    DEVICE_STATE_DEFAULT,
    DEVICE_STATE_ADDRESS,
    DEVICE_STATE_CONFIGURED
} usb_device_state_t;

static volatile usb_device_state_t device_state = DEVICE_STATE_UNATTACHED;

/*// USB Reset Interrupt Handler
ISR(USB_GEN_vect) {
    if (UDINT & (1 << EORSTI)) {
        UDINT &= ~(1 << EORSTI);
        timestamp_counter = 0;                // Reset timestamp on USB reset
        device_state = DEVICE_STATE_DEFAULT;
    }
}

// Start of Frame Interrupt Handler
ISR(USB_COM_vect) {
    if (UDINT & (1 << SOFI)) {
        UDINT &= ~(1 << SOFI);
        timestamp_counter++;                  // Increment timestamp counter
    }
}*/

// INIT SPI
void SPI_INIT2(void) {
	// The main question is whether or not DDRB controls the pins that these codenames wrap around. If not then we just need to also specify the correct version of DDRB
	DDRB |= (1 << pin_MOSI) | (1 << pin_SCK) | (1 << pin_SS); // Sets these pins as output pins for the purposes of SPI comms
	DDRB &= ~(1 << pin_MISO); // Set MISO as the input pins
	
	// GPT settings for SPI config. Need to see where this is coming from
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL); // Need to see where these config options came from in GPT
	SPSR = (1 << SPI2X);
}

uint8_t SPI_transmit2(uint8_t data) {
	SPDR = data;
	while (!(SPSR & (1 << SPIF))) {
		// DO NOTHING
	}
	return SPDR;
}

// SPI Initialization Function
void SPI_init(void) {
	// Set MOSI (PB2), SCK (PB1), and SS (PB3) as output pins
	DDRB |= (1 << PB2) | (1 << PB1) | (1 << PB3);

	// Set MISO (PB0) as an input pin
	DDRB &= ~(1 << PB0);

	// Configure SPI settings:
	// - Enable SPI: (1 << SPE)
	// - Master mode: (1 << MSTR)
	// - Clock polarity: CPOL = 1 (SCK idle high)
	// - Clock phase: CPHA = 0 (Mode 0, data sampled on rising edge, shifted out on falling edge)
	// - Clock speed: f_osc / 4 = 4 MHz (SPR1 = 0, SPR0 = 0, SPI2X = 1)
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPOL);  // Enable SPI, Master mode, CPOL = 1
	SPSR = (1 << SPI2X);                            // Enable double speed for 4 MHz SCK

	// Default to deselecting any connected SPI slaves (set SS high)
	PORTB |= (1 << PB3);
}

// SPI Transmit Function
uint8_t SPI_transmit(uint8_t data) {
	SPDR = data;                         // Load data into the SPI data register
	while (!(SPSR & (1 << SPIF))) {      // Wait for transmission complete
		// Do nothing
	}
	return SPDR;                         // Return received data
}

// If cond is true, then select the slave, if false then deselect the slave
void SPI_selection(bool cond) {
	if (cond) {
		PORTB &= ~ (1 << pin_SS);
	}
	
	else if (cond) {
		PORTB |= (1 << pin_SS);
	}
}

// This function does the delaying using Timer TC0 that is setup above this function.
void delay_ms(uint16_t milliseconds) {
	uint16_t i;

	for (i = 0; i  <milliseconds; i++) {
		TCNT0 = 0;
		while (!(TIFR0 &  (1 << OCF0A)));
		TIFR0 = (1 << OCF0A);
	}
}

// This function will light up LED "num" times in time [ms]
bool lightUp(uint8_t num, uint8_t GPIO_pin, uint16_t time) {
	uint16_t timer = time / num;

	for (int i = 0; i < num; i++) {
		PORTB |= (1 << GPIO_pin);
		delay_ms(timer/2);
		PORTB &= ! (1 << GPIO_pin);
		delay_ms(timer/2);
	}
	return true;
}

// Process voltage data and prepare USB packet
void process_voltage_data(void) {
    voltage_packet_t packet;
    
    // Fill packet with data
    packet.timestamp = timestamp_counter;
    
    /* Setup PD4 as input pin
    // DDRD |= (1 << 4); // Original: Set as output
    DDRD &= ~(1 << PD4);  // Set PD4 as input
    
     Read voltage values from probes via SPI
     for(uint8_t i = 0; i < 8; i++) {
         packet.probe_values[i] = SPI_transmit2(0xFF);  // Using existing SPI function
         packet.probe_values[i] = 2;
     } */

	uint16_t result = rand() % 1024;
    uint8_t voltage = (result * 5.0) / 1023.0;

    // Fill all probe values with the same GPIO reading
    for(uint8_t i = 0; i < 8; i++) {
        packet.probe_values[i] = voltage;
    }

    // Add to circular buffer if space available
    if (((buffer_head + 1) % BUFFER_SIZE) != buffer_tail) {
        memcpy((void*)&circular_buffer[buffer_head], &packet, sizeof(voltage_packet_t)); //hopefully this is good enough
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    }
}

// Main USB data transmission function
void USB_Task(void) {
    if (device_state == DEVICE_STATE_CONFIGURED && transfer_complete) {
        if (buffer_head != buffer_tail) {
            // Copy data from circular buffer to USB endpoint buffer
            memcpy(endpoint_buffer, (void*)&circular_buffer[buffer_tail], sizeof(voltage_packet_t));
            buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
            
            // Initiate USB bulk transfer
            UENUM = BULK_IN_EPADDR;
            UEINTX &= ~(1 << TXINI);
            transfer_complete = false;
        }
    }
}

int main(void) {
	USB_Init();                           // Initialize USB subsystem
	sei();                                // Enable global interrupts

	while (1) {

		/*SPI_select();                    // Select the SPI slave
		SPI_selection(true);
		uint8_t received = SPI_transmit2(0x55); // Transmit a byte and receive data
		SPI_selection(false);                  // Deselect the SPI slave */
		
		if (USB_DeviceState == DEVICE_STATE_CONFIGURED) {
			/*pin_SPI_PORT &= !(1 << pin_SS);
			SPDR = 0xAB;
			while (!(SPSR & (1 << SPIF)));
			uint8_t received = SPDR;
			lightUp(received, pin_LED_GPIO, 1000); If using SPI*/
			lightUp(2, pin_LED_GPIO, 1000);
			process_voltage_data();
			USB_Task();
		}
		
		else {
			lightUp(10, pin_LED_GPIO, 1000);
		}
	}

	return 0;
}