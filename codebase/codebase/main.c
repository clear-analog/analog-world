/*
 * codebase.c
 *
 * Created: 12/12/2024 4:04:31 PM
 * Author : suraj
 */ 

#include <avr/io.h>
#include "pin_mapping.h"

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

int main(void) {
	SPI_INIT2();                          // Initialize SPI

	while (1) {
		SPI_select();                    // Select the SPI slave
		uint8_t received = SPI_transmit2(0x55); // Transmit a byte and receive data
		SPI_deselect();                  // Deselect the SPI slave */
		
		// Process the received byte (optional)
		received = received + 1;
		// ...
	}

	return 0;
}
