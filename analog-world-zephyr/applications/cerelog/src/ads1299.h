#ifndef ADS1299_H
#define ADS1299_H

// Minimum necessary libraries
#include <stdint.h>
#include <zephyr/sys/util.h>

// Define the SPI frequency
#define ADS1299_SPI_FREQ 4000000

// --- Pin Mapping ---
static const uint8_t pin_MOSI_NUM = 23;
static const uint8_t pin_CS_NUM = 5;
static const uint8_t pin_MISO_NUM = 19;
static const uint8_t pin_SCK_NUM = 18;
static const uint8_t pin_PWDN_NUM = 13;
static const uint8_t pin_RST_NUM = 12;
static const uint8_t pin_START_NUM = 14;
static const uint8_t pin_DRDY_NUM = 27;
static const uint8_t pin_LED_DEBUG = 17;

// ADS1299 State Management
#define ADS1299_MODE_SDATAC 1
#define ADS1299_MODE_RDATAC 2

#define CMD_ADC_WREG 3
#define CMD_ADC_RREG 4
#define CMD_ADC_SDATAC 0x11
#define CMD_ADC_RDATAC 0x10
#define CMD_ADC_START  0x08

/* Current ADS1299 state */
static int ads1299_mode = -2; // init states before being manipulated
static int ads1299_prev_cmd = -1;

/* Unit Data Structure for controlling registers */
typedef struct {
    int add;
    int reg_val;
} regVal_pair;

/* List of registers to be set. If -2, end WREG. */
extern const regVal_pair ADS1299_REGISTER_LS[];

// Function declarations
struct ads1299_config {
    const struct device *zephyr_spi_dev;
    const struct spi_config *spi_cfg;
};

void ADS1299_WREG(uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config);
void ADS1299_RREG(uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config);
void ADS1299_SDATAC(const struct ads1299_config *config);
void ADS1299_RDATAC(const struct ads1299_config *config);
void ADS1299_START(const struct ads1299_config *config);
void ADS1299_RESET(const struct ads1299_config *config);
void ADS1299_WAKEUP(const struct ads1299_config *config);
void ADS1299_STANDBY(const struct ads1299_config *config);
int ADS1299_SETUP(const struct ads1299_config *config);
int ADS1299_READ_ID(uint8_t *id_val, const struct ads1299_config *config);
int ADS1299_SEND_CMD(uint8_t cmd, const struct ads1299_config *config);
int ADS1299_INIT(const struct ads1299_config *config);
static int ADS1299_REG_OPS(uint8_t opcode, uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config);

#endif // ADS1299_H