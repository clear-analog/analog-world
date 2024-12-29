/*
 * Descriptors.c
 *
 * Created: 12/28/2024 8:13:19 PM
 *  Author: suraj
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"

// Device Descriptor
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor = {
    .Header = {
        .Size = sizeof(USB_Descriptor_Device_t),
        .Type = DTYPE_Device
    },
    .USBSpecification = VERSION_BCD(2, 0, 0),
    .Class = 0xFF, // Vendor Specific
    .SubClass = 0xFF,
    .Protocol = 0xFF,
    .Endpoint0Size = 64,
    .VendorID = 0x03EB, // Atmel's VID
    .ProductID = 0x2048, // Arbitrary PID (should be registered)
    .ReleaseNumber = VERSION_BCD(1, 0, 0),
    .ManufacturerStrIndex = STRING_ID_Manufacturer,
    .ProductStrIndex = STRING_ID_Product,
    .SerialNumStrIndex = STRING_ID_Serial,
    .NumberOfConfigurations = 1
};

// Configuration Descriptor
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor = {
    .Config = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Configuration_Header_t),
            .Type = DTYPE_Configuration
        },
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
            .Type = DTYPE_Interface
        },
        .InterfaceNumber = 0,
        .AlternateSetting = 0,
        .TotalEndpoints = 1,
        .Class = 0xFF, // Vendor Specific
        .SubClass = 0xFF,
        .Protocol = 0xFF,
        .InterfaceStrIndex = NO_DESCRIPTOR
    },
    .DataInEndpoint = {
        .Header = {
            .Size = sizeof(USB_Descriptor_Endpoint_t),
            .Type = DTYPE_Endpoint
        },
        .EndpointAddress = ENDPOINT_DIR_IN | 1,
        .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize = 64,
        .PollingIntervalMS = 0x01
    }
};

// String Descriptors
const USB_Descriptor_String_t PROGMEM LanguageString = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);
const USB_Descriptor_String_t PROGMEM ManufacturerString = USB_STRING_DESCRIPTOR(L"Voltage Monitor");
const USB_Descriptor_String_t PROGMEM ProductString = USB_STRING_DESCRIPTOR(L"8-Channel Voltage Monitor");
const USB_Descriptor_String_t PROGMEM SerialString = USB_STRING_DESCRIPTOR(L"000001");

// Descriptor Callback Functions
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue, const uint16_t wIndex, const void** const DescriptorAddress) {
    const uint8_t DescriptorType = (wValue >> 8);
    const uint8_t DescriptorNumber = (wValue & 0xFF);
    
    void* Address = NULL;
    uint16_t Size = NO_DESCRIPTOR;
    
    switch (DescriptorType) {
        case DTYPE_Device:
            Address = (void*)&DeviceDescriptor;
            Size = sizeof(USB_Descriptor_Device_t);
            break;
        case DTYPE_Configuration:
            Address = (void*)&ConfigurationDescriptor;
            Size = sizeof(USB_Descriptor_Configuration_t);
            break;
        case DTYPE_String:
            switch (DescriptorNumber) {
                case STRING_ID_Language:
                    Address = (void*)&LanguageString;
                    Size = pgm_read_byte(&LanguageString.Header.Size);
                    break;
                case STRING_ID_Manufacturer:
                    Address = (void*)&ManufacturerString;
                    Size = pgm_read_byte(&ManufacturerString.Header.Size);
                    break;
                case STRING_ID_Product:
                    Address = (void*)&ProductString;
                    Size = pgm_read_byte(&ProductString.Header.Size);
                    break;
                case STRING_ID_Serial:
                    Address = (void*)&SerialString;
                    Size = pgm_read_byte(&SerialString.Header.Size);
                    break;
            }
            break;
    }
    
    *DescriptorAddress = Address;
    return Size;
}
