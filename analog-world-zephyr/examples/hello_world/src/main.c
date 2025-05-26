#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define MY_CUSTOM_STRING "Analog World Baby Girl"
int main(void) {

    uint32_t counter = 0;
    while (1) {
        printk("[%u] %s Time: %lld ms\n", counter++, MY_CUSTOM_STRING, k_uptime_get());
        k_msleep(1000);
    }
    return 0;
}