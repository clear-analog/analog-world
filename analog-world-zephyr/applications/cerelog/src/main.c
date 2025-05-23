#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h> // For DEVICE_DT_GET
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include "ads1299.h" // Your driver header

//static int init_spi(void);

// Check if node exists at compile time
#if DT_NODE_EXISTS(DT_NODELABEL(debug_led))
#pragma message "ledDebug exists!"
#else
#pragma message "pinPWDN does NOT exist!"
#endif

// Let's check what SPI nodes actually exist
#if DT_NODE_EXISTS(DT_NODELABEL(spi0))
#pragma message "spi0 exists"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(spi1))
#pragma message "spi1 exists"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(spi2))
#pragma message "spi2 exists"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(spi3))
#pragma message "spi3 exists"
#endif

// Check if the spi3 node has status okay
#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
#pragma message "spi3 status is okay"
#else
#pragma message "spi3 status is NOT okay"
#endif

/* SPI device and config for Zephyr */
//#define SPI_NODE DT_NODELABEL(spi3)

/* SPI Transaction Setup */
static struct spi_config spi_cfg = {
    .frequency = 4000000,
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER,
    .slave = 0,
    .cs = NULL
};


int main(void) {
    static const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi3));


    uint8_t tx_buffer[] = {0x01, 0x02, 0x03};
    uint8_t rx_buffer[3] = {0};

    /* Data Buffers to store info for SPI transactions */
    struct spi_buf tx_buf = {
        .buf = tx_buffer,
        .len = sizeof(tx_buffer)
    };

    /* Data Buffers to store info for SPI transactions */
    struct spi_buf rx_buf = {
        .buf = rx_buffer,
        .len = sizeof(rx_buffer)
    };

    /* GPIO pins config */
    struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};
    struct spi_buf_set rx_bufs = {.buffers = &rx_buf, .count = 1};

    int ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
    spi_write(spi_dev, &spi_cfg, &tx_bufs);
    //if (ret < 0) {
        printk("SPI Transaction failed\n");
    //}

    return 0;
}