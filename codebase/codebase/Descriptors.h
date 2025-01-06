/*
 * Descriptors.h
 *
 * Created: 1/4/2025 8:27:38 PM
 *  Author: suraj
 */ 


#ifndef DESCRIPTORS_H_
#define DESCRIPTORS_H_

	/* Things to Include */
	#include <LUFA/Drivers/USB/USB.h>
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/power.h>
	#include <avr/pgmspace.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdbool.h>
	#include <util/delay.h>
	
	// Data structure containing the message
	typedef struct {
		float data[8];
		uint32_t timestamp;
	} SensorData_t;
	
	// String Descriptor IDs
	enum StringDescriptors {
		STRING_ID_Language     = 0,
		STRING_ID_Manufacturer = 1,
		STRING_ID_Product      = 2,
		STRING_ID_Serial       = 3,
	};

	// Device/Interface Class IDs
	#define DEVICE_CLASS                USB_CSCP_VendorSpecificClass
	#define DEVICE_SUBCLASS             USB_CSCP_NoDeviceSubclass
	#define DEVICE_PROTOCOL             USB_CSCP_NoDeviceProtocol

	#define INTERFACE_CLASS             USB_CSCP_VendorSpecificClass
	#define INTERFACE_SUBCLASS          0x00
	#define INTERFACE_PROTOCOL          0x00

	// Endpoint Addresses
	#define BULK_IN_EPADDR             (ENDPOINT_DIR_IN | 1)
	#define BULK_OUT_EPADDR            (ENDPOINT_DIR_OUT | 2)

	// Endpoint Sizes
	#define BULK_EPSIZE                64  // 64 bytes for Full Speed USB

	// USB Device Descriptor
	typedef struct {
		USB_Descriptor_Configuration_Header_t Config;
		USB_Descriptor_Interface_t            Interface;
		USB_Descriptor_Endpoint_t            DataInEndpoint;
		USB_Descriptor_Endpoint_t            DataOutEndpoint;
	} USB_Descriptor_Configuration_t;

	uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
	const uint16_t wIndex,
	const void** const DescriptorAddress)
	ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif /* DESCRIPTORS_H_ */