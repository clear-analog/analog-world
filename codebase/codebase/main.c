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
#include "Descriptors.h"
#include "pin_mapping.h"

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

// Data packet structure
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

/*/ Initialize USB subsystem
void USB_Init(void) {
    USBCON = (1 << USBE) | (1 << FRZCLK);    // Enable USB controller
    PLLCSR = (1 << PLLE) | (1 << PLLP0);     // Configure PLL
    while (!(PLLCSR & (1 << PLOCK)));        // Wait for PLL lock
    USBCON = (1 << USBE);                    // Enable USB
    UDCON &= ~(1 << DETACH);                 // Attach USB
    UDIEN = (1 << EORSTE) | (1 << SOFE);     // Enable interrupts
    sei();                                    // Enable global interrupts
}*/

// USB Reset Interrupt Handler
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
}
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

	// Default to deselecting any conneted SPI slaves (set SS high)
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

// SPI Select/Deselect Slave Functions
void SPI_select(void) {
	PORTB &= ~(1 << PB3);                // Pull SS low to select the slave
}

void SPI_deselect(void) {
	PORTB |= (1 << PB3);                 // Pull SS high to deselect the slave
}

// Setting something to ouptput
void pin4_output(void) {
	DDRD |= (1 << 4); //0b0000 0001 << 4 ==> 0b0001 0000
}

// Process voltage data and prepare USB packet
void process_voltage_data(void) {
    voltage_packet_t packet;
    
    // Fill packet with data
    packet.timestamp = timestamp_counter;
    
    // Read voltage values from probes via SPI
    for(uint8_t i = 0; i < 8; i++) {
        packet.probe_values[i] = SPI_transmit2(0xFF);  // Using existing SPI function
    }
    
    // Add to circular buffer if space available
    if (((buffer_head + 1) % BUFFER_SIZE) != buffer_tail) {
        memcpy((void*)&circular_buffer[buffer_head], &packet, sizeof(voltage_packet_t));
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    }
}

// Main USB data transmission function
void USB_Task(void) {
    if (device_state == DEVICE_STATE_CONFIGURED && transfer_complete) {
        if (buffer_head != buffer_tail) {
            // Prepare and send USB packet
            memcpy(endpoint_buffer, (void*)&circular_buffer[buffer_tail], BULK_IN_EPSIZE);
            buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
            
            // Initiate USB transfer
            UENUM = BULK_IN_EPADDR;
            UEINTX &= ~(1 << TXINI);
            transfer_complete = false;
        }
    }
}



// Delay function that waits for specified number of clock cycles
void delay_cycles(uint16_t cycles) {
    // Each iteration of this loop takes 4 clock cycles
    // The division by 4 converts desired cycles into loop iterations needed
    uint16_t iterations = cycles / 4;
    
    // Using inline assembly for precise cycle control
    // The empty asm volatile ensures compiler doesn't optimize it away
    while (iterations > 0) {
        asm volatile("nop"); // 1 cycle no-operation
        iterations--;        // 3 cycles for decrement and branch
    }
}


int main(void) {
	SPI_INIT2();                          // Initialize SPI

	while (1) {
		SPI_select();                    // Select the SPI slave
		uint8_t received = SPI_transmit2(0x55); // Transmit a byte and receive data
		SPI_deselect();                  // Deselect the SPI slave */
		
		// Process the received byte (optional)
		received = received + 1;
		PORTD = 0b00100000;
		// ...
	}

	return 0;
}
