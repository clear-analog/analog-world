/*
 * Descriptors.c
 *
 * Created: 1/4/2025 8:41:19 PM
 *  Author: suraj
 */ 
	#include <avr/io.h>
	#include <avr/pgmspace.h>
	#include <LUFA/Drivers/USB/USB.h>
	#include "Descriptors.h"

	// Device Descriptor - Critical for device enumeration
	const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Device_t),
			.Type = DTYPE_Device},
		.USBSpecification = VERSION_BCD(2,0,0),
		.Class = USB_CSCP_VendorSpecificClass,
		.SubClass = USB_CSCP_NoDeviceSubclass,
		.Protocol = USB_CSCP_NoDeviceProtocol,
		.Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,  // Must be 64 for USB 2.0
		.VendorID = 0x03EB,  // Must be valid VID
		.ProductID = 0x2048,  // Must be valid PID
		.ReleaseNumber = VERSION_BCD(1,0,0),
		.ManufacturerStrIndex = STRING_ID_Manufacturer, // Must point to valid string
		.ProductStrIndex = STRING_ID_Product, // Must point to valid string  
		.SerialNumStrIndex = STRING_ID_Serial, // Serial number is required for Windows
		.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
	};

	// Configuration Descriptor - Critical for device functionality
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type = DTYPE_Configuration},
        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
        .TotalInterfaces = 1,
        .ConfigurationNumber = 1,
        .ConfigurationStrIndex = NO_DESCRIPTOR,
        .ConfigAttributes = USB_CONFIG_ATTR_RESERVED,
        .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)
    },
    .Interface = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Interface_t),
            .Type = DTYPE_Interface},
        .InterfaceNumber = 0,
        .AlternateSetting = 0,
        .TotalEndpoints = 2,  // Changed to 2 endpoints (IN and OUT)
        .Class = INTERFACE_CLASS,      // Using constant from header
        .SubClass = INTERFACE_SUBCLASS, // Using constant from header
        .Protocol = INTERFACE_PROTOCOL, // Using constant from header
        .InterfaceStrIndex = NO_DESCRIPTOR
    },
    .DataInEndpoint = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Endpoint_t),
            .Type = DTYPE_Endpoint
        },
        .EndpointAddress = BULK_IN_EPADDR,  // Using constant from header
        .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize = BULK_EPSIZE,  // Using constant from header
        .PollingIntervalMS = 0x01
    },
    .DataOutEndpoint = {  // Added OUT endpoint
        .Header = {
            .Size = sizeof(USB_Descriptor_Endpoint_t),
            .Type = DTYPE_Endpoint
        },
        .EndpointAddress = BULK_OUT_EPADDR,  // Using constant from header
        .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize = BULK_EPSIZE,  // Using constant from header
        .PollingIntervalMS = 0x01
    }
};

// String Descriptors
const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Voltage Monitor");
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"8-Channel Voltage Monitor");
const USB_Descriptor_String_t PROGMEM SerialString = USB_STRING_DESCRIPTOR(L"000001");

// String Descriptor handling is critical for Windows enumeration
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
const uint16_t wIndex,
const void** const DescriptorAddress) {
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);
	const void* Address = NULL;
	uint16_t    Size    = NO_DESCRIPTOR;

	switch (DescriptorType) {
		case DTYPE_Device:
			Address = &DeviceDescriptor;
			Size    = sizeof(USB_Descriptor_Device_t);
			break;
		case DTYPE_Configuration:
			Address = &ConfigurationDescriptor;
			Size    = sizeof(USB_Descriptor_Configuration_t);
			break;
		case DTYPE_String:
			switch (DescriptorNumber) {
				case STRING_ID_Language:
					Address = &LanguageString;
					Size    = pgm_read_byte(&LanguageString.Header.Size);
					break;
				case STRING_ID_Manufacturer:
					Address = &ManufacturerString;
					Size    = pgm_read_byte(&ManufacturerString.Header.Size);
					break;
				case STRING_ID_Product:
					Address = &ProductString;
					Size    = pgm_read_byte(&ProductString.Header.Size);
					break;
				case STRING_ID_Serial:
					Address = &SerialString;
					Size    = pgm_read_byte(&SerialString.Header.Size);
					break;
			}
			break;
	}

	*DescriptorAddress = Address;
	return Size;
}