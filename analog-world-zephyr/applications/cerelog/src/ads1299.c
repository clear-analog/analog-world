#include "ads1299.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>

/* List of registers to be set. If -2, end WREG. */
const regVal_pair ADS1299_REGISTER_LS[] = {
    {0x01, 0b10110000},  // CONFIG1: Data rate 250 SPS
    {0x02, 0b11010000},  // CONFIG2: Internal test signal enabled
    {0x03, 0b11101100},  // CONFIG3: Reference buffer enabled, bias enabled
    {0x04, 0},           // LOFF: Lead-off detection disabled
    {-2, -2},            // End of first section
    {0x05, 0b01100000},  // CH1SET: Gain 12, normal input
    {0x06, 0b01100000},  // CH2SET: Gain 12, normal input
    {0x07, 0b01100000},  // CH3SET: Gain 12, normal input
    {0x08, 0b01100000},  // CH4SET: Gain 12, normal input
    {0x09, 0b01100000},  // CH5SET: Gain 12, normal input
    {0x0A, 0b01100000},  // CH6SET: Gain 12, normal input
    {0x0B, 0b01100000},  // CH7SET: Gain 12, normal input
    {0x0C, 0b01100000},  // CH8SET: Gain 12, normal input
    {0x0D, 0b11111111},  // BIAS_SENSP: All positive channels for bias
    {0x0E, 0b11111111},  // BIAS_SENSN: All negative channels for bias
    {0x0F, 0},           // LOFF_SENSP: Lead-off sensing disabled
    {0x10, 0},           // LOFF_SENSN: Lead-off sensing disabled
    {0x11, 0},           // LOFF_FLIP: No lead-off current direction flip
    {-2, -2},            // End of second section
    {0x15, 0},           // MISC1: SRB1 disabled
    {0x16, 0},           // MISC2: Reserved
    {0x17, 0},           // CONFIG4: Continuous conversion mode
    {0x02, 0xD0},        // CONFIG2: Test signal settings
    {0x05, 0x05},        // CH1SET: Test signal input
    {0x06, 0x05}         // CH2SET: Test signal input
};

/* This is not an init function that lives in ads1299.c.
   This file is blessed to sift necessary information from main.c to fill the vessels zephyr_dev and spi_cfg*/
int ADS1299_INIT(const struct ads1299_config *config) {
    if (!config || !config->zephyr_spi_dev || !config->spi_cfg) {
        return -EINVAL;
    }
    // No need to store zephyr_dev or spi_cfg, just use config directly
    return 0;
}

/* Internal function to send SPI command */
int ADS1299_SEND_CMD(uint8_t cmd, const struct ads1299_config *config) {
    struct spi_buf tx_buf = {
        .buf = &cmd,
        .len = 1
    };
    struct spi_buf_set tx_bufs = {
        .buffers = &tx_buf,
        .count = 1
    };

    int ret = spi_write(config->zephyr_spi_dev, config->spi_cfg, &tx_bufs);

    if (ret != 0) {
        printk("Failed to send command 0x%02x: %d\n", cmd, ret);
        return ret;
    }

    ads1299_prev_cmd = cmd;
    k_usleep(4 * 1000000 / ADS1299_SPI_FREQ); // Wait 4 tCLK cycles

    return 0;
}

/* Internal function for register operations */
static int ADS1299_REG_OPS(uint8_t opcode, uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config) {
    uint8_t cmd_buf[2];
    struct spi_buf tx_bufs_arr[3];
    struct spi_buf_set tx_bufs;
    int buf_count = 0;

    // First command byte: opcode + register address
    cmd_buf[0] = opcode | (reg_addr & 0x1F);
    tx_bufs_arr[buf_count].buf = &cmd_buf[0];
    tx_bufs_arr[buf_count].len = 1;
    buf_count++;

    // Second command byte: number of registers - 1
    cmd_buf[1] = len - 1;
    tx_bufs_arr[buf_count].buf = &cmd_buf[1];
    tx_bufs_arr[buf_count].len = 1;
    buf_count++;

    // For write operations, add data
    if ((opcode & 0x60) == 0x40) { // WREG command
        tx_bufs_arr[buf_count].buf = data;
        tx_bufs_arr[buf_count].len = len;
        buf_count++;
    }

    tx_bufs.buffers = tx_bufs_arr;
    tx_bufs.count = buf_count;

    int ret = spi_write(config->zephyr_spi_dev, config->spi_cfg, &tx_bufs);
    if (ret != 0) {
        printk("Failed register operation: %d\n", ret);
        return ret;
    }

    k_usleep(4 * 1000000 / ADS1299_SPI_FREQ); // Wait 4 tCLK cycles

    // For read operations, read the data
    if ((opcode & 0x60) == 0x20) { // RREG command
        struct spi_buf rx_buf = {
            .buf = data,
            .len = len
        };
        struct spi_buf_set rx_bufs = {
            .buffers = &rx_buf,
            .count = 1
        };

        ret = spi_read(config->zephyr_spi_dev, config->spi_cfg, &rx_bufs);
        if (ret != 0) {
            printk("Failed to read register data: %d\n", ret);
            return ret;
        }
    }

    return 0;
}

void ADS1299_WREG(uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config) {
    if (ads1299_mode == ADS1299_MODE_RDATAC) {
        printk("Warning: Cannot write registers in RDATAC mode\n");
        return;
    }

    ADS1299_REG_OPS(0x40, reg_addr, data, len, config);
    ads1299_prev_cmd = CMD_ADC_WREG;
}

void ADS1299_RREG(uint8_t reg_addr, uint8_t *data, uint8_t len, const struct ads1299_config *config) {
    if (ads1299_mode == ADS1299_MODE_RDATAC) {
        printk("Warning: Cannot read registers in RDATAC mode\n");
        return;
    }

    ADS1299_REG_OPS(0x20, reg_addr, data, len, config);
    ads1299_prev_cmd = CMD_ADC_RREG;
}

void ADS1299_SDATAC(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(CMD_ADC_SDATAC, config);
    ads1299_mode = ADS1299_MODE_SDATAC;
    printk("ADS1299 set to SDATAC mode\n");
}

void ADS1299_RDATAC(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(CMD_ADC_RDATAC, config);
    ads1299_mode = ADS1299_MODE_RDATAC;
    printk("ADS1299 set to RDATAC mode\n");
}

void ADS1299_START(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(CMD_ADC_START, config);
    printk("ADS1299 START command sent\n");
}

void ADS1299_RESET(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(0x06, config); // RESET command
    k_msleep(18 * 1000000 / ADS1299_SPI_FREQ); // Wait 18 tCLK cycles
    ads1299_mode = -2;
    printk("ADS1299 RESET command sent\n");
}

void ADS1299_WAKEUP(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(0x02, config); // WAKEUP command
    printk("ADS1299 WAKEUP command sent\n");
}

void ADS1299_STANDBY(const struct ads1299_config *config) {
    ADS1299_SEND_CMD(0x04, config); // STANDBY command
    printk("ADS1299 STANDBY command sent\n");
}

int ADS1299_SETUP(const struct ads1299_config *config) {
    int ret;
    uint8_t reg_val;

    printk("Configuring ADS1299 registers...\n");

    // Make sure we're in SDATAC mode
    if (ads1299_mode != ADS1299_MODE_SDATAC) {
        ADS1299_SDATAC(config);
        k_msleep(1);
    }

    // Configure registers according to the register list
    for (int i = 0; i < ARRAY_SIZE(ADS1299_REGISTER_LS); i++) {
        const regVal_pair reg_pair = ADS1299_REGISTER_LS[i];

        // Check for end marker
        if (reg_pair.add == -2) {
            printk("Register configuration section %d complete\n", i);
            k_msleep(1); // Small delay between sections
            continue;
        }

        reg_val = (uint8_t)reg_pair.reg_val;
        ADS1299_WREG(reg_pair.add, &reg_val, 1, config);

        // Verify the write by reading back
        uint8_t readback_val = 0;
        ADS1299_RREG(reg_pair.add, &readback_val, 1, config);

        if (readback_val != reg_val) {
            printk("Register 0x%02x verification failed: wrote 0x%02x, read 0x%02x\n",
                   reg_pair.add, reg_val, readback_val);
            // Continue with other registers - don't fail completely
        } else {
            printk("Register 0x%02x = 0x%02x OK\n", reg_pair.add, reg_val);
        }

        k_msleep(1);
    }

    printk("ADS1299 register configuration complete\n");
    return 0;
}

// Function to read ADS1299 ID register for verification
int ADS1299_READ_ID(uint8_t *id_val, const struct ads1299_config *config) {
    if (ads1299_mode == ADS1299_MODE_RDATAC) {
        ADS1299_SDATAC(config);
        k_msleep(1);
    }

    ADS1299_RREG(0x00, id_val, 1, config); // ID register is at address 0x00

    printk("ADS1299 ID register: 0x%02x\n", *id_val);

    // Decode ID register
    uint8_t rev_id = (*id_val >> 5) & 0x7;
    uint8_t dev_id = (*id_val >> 2) & 0x3;
    uint8_t nu_ch = *id_val & 0x3;

    printk("  Revision ID: %d\n", rev_id);
    printk("  Device ID: %d\n", dev_id);
    printk("  Number of channels: %d\n", (nu_ch == 0) ? 4 : (nu_ch == 1) ? 6 : 8);

    return 0;
}