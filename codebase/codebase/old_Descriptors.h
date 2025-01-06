/*
 * Descriptors.h
 *
 * Created: 12/28/2024 8:14:23 PM
 *  Author: suraj
 */ 


#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_

	/* Includes: */
	#include <LUFA/Drivers/USB/USB.h>
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <util/delay.h>

// USB Device Descriptor
typedef struct {
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t            Interface;
    USB_Descriptor_Endpoint_t            DataInEndpoint;
} USB_Descriptor_Configuration_t;

// Device/Interface Class IDs
#define DEVICE_CLASS                0xFF    // Vendor Specific
#define DEVICE_SUBCLASS             0x00
#define DEVICE_PROTOCOL             0x00

#define INTERFACE_CLASS             0xFF    // Vendor Specific  
#define INTERFACE_SUBCLASS          0x00
#define INTERFACE_PROTOCOL          0x00

// Endpoint Addresses
#define BULK_IN_EPADDR             (ENDPOINT_DIR_IN  | 1)

// Endpoint Sizes
#define BULK_IN_EPSIZE             64      // 64 bytes for Full Speed USB

// String Descriptor IDs
enum StringDescriptors {
    STRING_ID_Language     = 0,
    STRING_ID_Manufacturer = 1, 
    STRING_ID_Product      = 2,
    STRING_ID_Serial       = 3,
};

// Function Prototypes
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                  const uint16_t wIndex,
                                  const void** const DescriptorAddress)
                                  ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);




#endif /* DESCRIPTORS_H_ */