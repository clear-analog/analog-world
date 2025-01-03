/*
 * LUFAConfig.h
 *
 * Created: 12/28/2024 8:19:27 PM
 *  Author: suraj
 */ 


#ifndef LUFACONFIG_H_
#define LUFACONFIG_H_
	#if (ARCH == ARCH_AVR8)

		// LUFA Configuration Options
		#define USE_STATIC_OPTIONS         (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)

		// USB Device Mode Options
		#define USB_DEVICE_ONLY
		#define USE_FLASH_DESCRIPTORS
		#define FIXED_CONTROL_ENDPOINT_SIZE      64
		#define FIXED_NUM_CONFIGURATIONS         1
		#define MAX_ENDPOINT_INDEX              1

		// USB Device Class Driver Options
		#define HID_HOST_BOOT_PROTOCOL_ONLY
		#define HID_ENDPOINT_POLLING_INTERVAL    0x01

		// USB Endpoint Sizes
		#define BULK_IN_EPSIZE                  64

		// LUFA Debugging Options
		#define NO_LIMITED_CONTROLLER_CONNECT
		#define NO_SOF_EVENTS

		// LUFA Feature Options
		#define NO_INTERNAL_SERIAL
		#define NO_DEVICE_REMOTE_WAKEUP
		#define NO_DEVICE_SELF_POWER

		// LUFA Buffer Options
		#define USB_STREAM_TIMEOUT_MS            100
		#define RING_BUFFER_SIZE                 256
		#define INTERRUPT_CONTROL_ENDPOINT		1
#endif


#endif /* LUFACONFIG_H_ */