#ifndef ADS1299_H
#define ADS1299_H

// Minimum necessary libraries
#include <stdint.h>

// Define the SPI frequency
#define ADS1299_SPI_FREQ 4000000

// --- Pin Mapping ---
static const uint8_t pin_MOSI_NUM = 23;
static const uint8_t pin_CS_NUM = 5;
static const uint8_t pin_MISO_NUM = 19;
static const uint8_t pin_SCK_NUM = 18; // this must change. Double check since there are two mappings rn
static const uint8_t pin_PWDN_NUM = 13;
static const uint8_t pin_RST_NUM = 12;
static const uint8_t pin_START_NUM = 14;
static const uint8_t pin_DRDY_NUM = 27;
static const uint8_t pin_LED_DEBUG = 17;

// ADS1299 State Management
int _ADS1299_MODE = -2;
int ADS1299_MODE_SDATAC = 1;
int ADS1299_MODE_RDATAC = 2;

int _ADS1299_PREV_CMD = -1;
int CMD_ADC_WREG = 3;
int CMD_ADC_RREG = 4;
int CMD_ADC_SDATAC = 0x11;
int CMD_ADC_RDATAC = 0x10;
int CMD_ADC_START  = 0x08;

/* Unit Data Structure for controlling registers */
typedef struct Deez {
    int add;
    int reg_val;
} regVal_pair;

/* List of registers to be set. If -2, end WREG. */
static const regVal_pair ADS1299_REGISTER_LS[29] = {
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
    {0x17, 0},
    {0x02, 0xD0},
    {0x05, 0x05},
    {0x06, 0x05}
};

// Function declarations
void ADS1299_WREG(uint8_t, uint8_t*, uint8_t);
void ADS1299_RREG(uint8_t, uint8_t*, uint8_t);
void ADS1299_SDATAC(void);
void ADS1299_RDATAC(void);
void ADS1299_START(void);
void ADS1299_RESET(void);
void ADS1299_WAKEUP(void);
void ADS1299_STANDBY(void);
void ADS1299_SETUP(void);

#endif