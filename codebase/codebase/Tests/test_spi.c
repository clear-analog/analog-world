/*
 * test_spi.c
 *
 * Created: 1/14/2025 2:02:13 PM
 *  Author: suraj
 */ 

#include "../Config/AppConfig.h"

int test_spi(void) {
    // Check that SS pin works
    lightUp(1, pin_SS, 1000);
    lightUp(2, pin_SS, 1000);
    lightUp(3, pin_SS, 1000);

    // Write 20 into a particular register
    regVal_pair store; store.add = 0x05; store.reg_val = 20;
    uint8_t value[] = {(uint8_t)store.reg_val};
    ADS1299_WREG((uint8_t)store.add, value, 1);

    // Read the register which we wrote 20 to
    ADS1299_RREG((uint8_t)store.add, value, 1);

    // Display this register value on the Debug LED
    lightUp(value[0], pin_LED_DEBUG, 1000);

    // Fix the register value
    value[0] = (uint8_t) 0b01100000;
    ADS1299_WREG((uint8_t)store.add, value, 1);

    return 0;
}