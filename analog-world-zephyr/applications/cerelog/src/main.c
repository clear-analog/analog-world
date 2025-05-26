#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "ads1299.h"
//#include "data_handler.h"

// GPIO Pin definitions
#define ADS1299_PWDN_PIN    13
#define ADS1299_RST_PIN     12
#define ADS1299_START_PIN   14
#define ADS1299_DRDY_PIN    27

// Data buffer for ADS1299 samples
static uint8_t ads_raw_data[27]; // 24 bits status + 24*8 bits data = 216 bits = 27 bytes
//static ads1299_sample_t current_sample;

// USB/UART transmission buffer
static uint8_t tx_buffer[64];

// Forward declarations
static void drdy_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static int ads1299_init_device(const struct device *gpio_dev, const struct ads1299_config *ads1299_cfg);
static int ads1299_read_data(const struct ads1299_config *ads1299_cfg);
//static void data_acquisition_thread(const struct ads1299_config *ads1299_cfg);
//static void usb_transmission_thread(void);

/* // Thread definitions
K_THREAD_DEFINE(acq_thread, 2048, data_acquisition_thread, NULL, NULL, NULL, 
                K_PRIO_COOP(5), 0, 0);
 K_THREAD_DEFINE(usb_thread, 2048, usb_transmission_thread, NULL, NULL, NULL, 
                K_PRIO_COOP(7), 0, 0);

// Semaphores for thread synchronization
K_SEM_DEFINE(data_ready_sem, 0, 1); // this is DRDY pin part
K_SEM_DEFINE(usb_ready_sem, 0, 10);

static void drdy_interrupt_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);
    
    data_ready = true;
    k_sem_give(&data_ready_sem);
} */

static int ads1299_init_device(const struct device *gpio_dev, const struct ads1299_config *ads1299_cfg) {
    int ret;

    printk("Initializing ADS1299...\n");

    // Power down sequence
    gpio_pin_set(gpio_dev, ADS1299_PWDN_PIN, 0);
    k_msleep(1);
    gpio_pin_set(gpio_dev, ADS1299_RST_PIN, 0);
    k_msleep(1);

    // Power up sequence
    gpio_pin_set(gpio_dev, ADS1299_PWDN_PIN, 1);
    k_msleep(10);
    gpio_pin_set(gpio_dev, ADS1299_RST_PIN, 1);
    k_msleep(100); // Wait for power-on reset

    printk("No issue with gpio manipulation\n");

    // Test: Check if SPI config is valid
    printk("SPI device: %p\n", ads1299_cfg->zephyr_spi_dev);
    printk("SPI config: %p\n", ads1299_cfg->spi_cfg);
    printk("CS GPIO port: %p\n", ads1299_cfg->spi_cfg->cs.gpio.port);
    printk("CS GPIO pin: %d\n", ads1299_cfg->spi_cfg->cs.gpio.pin);

    // COMMENT OUT THE CRASHING CALLS FOR NOW
    ADS1299_SDATAC(ads1299_cfg);
    printk("Skipped ADS1299_SDATAC for now\n");

    // ret = ADS1299_SETUP(ads1299_cfg);
    printk("Skipped ADS1299_SETUP for now\n");

    // ADS1299_RDATAC(ads1299_cfg);
    printk("Skipped ADS1299_RDATAC for now\n");

    printk("ADS1299 initialization complete\n");
    return 0;
}


/* static int ads1299_read_data(const struct ads1299_config *ads1299_cfg) {
    struct spi_buf rx_buf = {
        .buf = ads_raw_data,
        .len = sizeof(ads_raw_data)
    };
    struct spi_buf_set rx_bufs = {
        .buffers = &rx_buf,
        .count = 1
    };
    
    // Read data from ADS1299
    int ret = spi_read(ads1299_cfg->zephyr_spi_dev, ads1299_cfg->spi_cfg, &rx_bufs);
    if (ret != 0) {
        printk("SPI read failed: %d\n", ret);
        return ret;
    }
    
    return 0;
}

static void data_acquisition_thread(const struct ads1299_config *ads1299_cfg) {
    int ret;

    printk("Data acquisition thread started\n");

    while (1) {
        // Wait for DRDY interrupt
        k_sem_take(&data_ready_sem, K_FOREVER);

        if (!acquisition_active) {
            continue;
        }

        // Read data from ADS1299
        ret = ads1299_read_data(ads1299_cfg);
        if (ret == 0) {
            // Process raw data into structured format
            process_ads1299_data(ads_raw_data, &current_sample);

            // Signal USB thread that new data is ready
            k_sem_give(&usb_ready_sem);
        }

        data_ready = false;
        printk("I wuz here");
    }
} */

/*static void usb_transmission_thread(void) {
    int ret;
    size_t tx_len;
    
    printk("USB transmission thread started\n");
    
    while (1) {
        // Wait for new sample data
        k_sem_take(&usb_ready_sem, K_FOREVER);
        
        // Format data for transmission
        tx_len = format_sample_for_transmission(&current_sample, tx_buffer, sizeof(tx_buffer));
        
        if (tx_len > 0) {
            // Send data via UART (USB CDC)
            for (size_t i = 0; i < tx_len; i++) {
                uart_poll_out(uart_dev, tx_buffer[i]);
                printk("%d\n", tx_buffer[i]);
            }
        }
    }
} */

int main(void) {
    int ret;
    k_msleep(2000);
    printk("Basic Test Starting...\n");
    
    // Get device handles
    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return -1;
    }
    
    const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio_dev)) {
        printk("GPIO device not ready\n");
        return -1;
    }

    // Configure all GPIO pins
    ret = gpio_pin_configure(gpio_dev, ADS1299_PWDN_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("Failed to configure PWDN pin: %d\n", ret);
        return ret;
    }
    
    ret = gpio_pin_configure(gpio_dev, ADS1299_RST_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("Failed to configure RST pin: %d\n", ret);
        return ret;
    }
    
    ret = gpio_pin_configure(gpio_dev, ADS1299_START_PIN, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("Failed to configure START pin: %d\n", ret);
        return ret;
    }
    
    ret = gpio_pin_configure(gpio_dev, ADS1299_DRDY_PIN, GPIO_INPUT | GPIO_INT_EDGE_FALLING);
    if (ret != 0) {
        printk("Failed to configure DRDY pin: %d\n", ret);
        return ret;
    }
    
    printk("All GPIO pins configured successfully\n");

    //const struct gpio_dt_spec cs_gpio = GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi3), cs_gpios, 0);
    
    const struct gpio_dt_spec cs_gpio = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 5,
    .dt_flags = GPIO_ACTIVE_LOW
};
    printk("Got CS_GPIO no prob\n");
    struct spi_config ads1299_spi_cfg = {
        .frequency = 4000000,
        .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_MODE_CPHA | SPI_OP_MODE_MASTER,
        //.slave = 0,
        .cs = {
            .gpio = &cs_gpio,
            .delay = 1
        }
    };
    const struct device *spi_zephyr_dev = DEVICE_DT_GET(DT_NODELABEL(spi3));
    printk("Got the SPI ZEPHYR DEVICE no prob");
    if (!device_is_ready(spi_zephyr_dev)) {
        printk("SPI device not ready\n");
        return -1;
    }

/*     // With this:
    struct ads1299_config ads1299_config = {
        .zephyr_spi_dev = spi_zephyr_dev,  // Note: no & needed since spi_zephyr_dev is already a pointer
        .spi_cfg = &ads1299_spi_cfg
    }; */

    struct ads1299_config ads1299_cfg = {
        .zephyr_spi_dev = spi_zephyr_dev,
        .spi_cfg = &ads1299_spi_cfg
    };

    printk("SPI device ready\n");
    ret = ADS1299_INIT(&ads1299_cfg);
    printk("ADS1299 Driver Init done");

    // Initialize ADS1299
    ret = ads1299_init_device(gpio_dev, &ads1299_cfg);
    if (ret != 0) {
        printk("Failed to initialize ADS1299: %d\n", ret);
        return ret;
    }
    
/*     // Setup DRDY interrupt
    gpio_init_callback(&drdy_cb_data, drdy_interrupt_handler, BIT(ADS1299_DRDY_PIN));
    ret = gpio_add_callback(gpio_dev, &drdy_cb_data);
    if (ret != 0) {
        printk("Failed to add GPIO callback: %d\n", ret);
        return ret;
    }
    
    ret = gpio_pin_interrupt_configure(gpio_dev, ADS1299_DRDY_PIN, GPIO_INT_EDGE_FALLING);
    if (ret != 0) {
        printk("Failed to configure GPIO interrupt: %d\n", ret);
        return ret;
    }

    printk("Interrupt configured\n"); */

    // Main loop
    while (1) {
        k_msleep(1000);
        printk("System running...\n");
        printk(ADS1299_SEND_CMD(0x00, &ads1299_cfg));
    }
    
    return 0;    
    /*
    // Setup DRDY interrupt
    gpio_init_callback(&drdy_cb_data, drdy_interrupt_handler, BIT(ADS1299_DRDY_PIN));
    ret = gpio_add_callback(gpio_dev, &drdy_cb_data);
    if (ret != 0) {
        printk("Failed to add GPIO callback: %d\n", ret);
        return ret;
    }
    
    ret = gpio_pin_interrupt_configure(gpio_dev, ADS1299_DRDY_PIN, GPIO_INT_EDGE_FALLING);
    if (ret != 0) {
        printk("Failed to configure GPIO interrupt: %d\n", ret);
        return ret;
    }

    struct ads1299_config ads_config = {
        .spi_zephyr_dev = spi_zephyr_dev,
        .spi_cfg = &ads1299_spi_cfg
    };
    
    ret = ads1299_init(&ads_config);
    if (ret != 0) {
        printk("Failed to initialize ADS1299 driver: %d\n", ret);
        return ret;
    }
    

    
    printk("System initialized successfully. Starting data acquisition...\n");
    
    // Main loop - can be used for status monitoring or commands
    while (1) {
        k_msleep(1000);
        printk("Cool world\n");
        // Could add status printing, command processing, etc.
    }
    */
}