/*
 * main.c
 *
 * Created: 1/4/2025 8:31:37 PM
 *  Author: suraj
 */ 

#include "Descriptors.h"
#include "Config/AppConfig.h"
#include "Config/LUFAConfig.h"
#include <stdlib.h>

static SensorData_t sensorData;
static bool device_configured = false;

int main(void) {
	// System initialization
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	clock_prescale_set(clock_div_1);

	USB_Init();
	sei();
	long int countter = 0;

	while (1) {
		// Your sensor data acquisition code here
		// Update sensorData.data[] and sensorData.timestamp
		sensorData.timestamp = ++countter;
		for (int i = 0; i < 8; i++) {
			uint8_t randVal = rand() & 0xFF;
			sensorData.data[i] = (float)randVal;
		}
		SendSensorData();
		USB_USBTask();
	}
}

void EVENT_USB_Device_ConfigurationChanged(void) {
	device_configured = true;
	Endpoint_ConfigureEndpoint(ENDPOINT_DIR_IN | 1, EP_TYPE_BULK, 64, 1);
	Endpoint_ConfigureEndpoint(ENDPOINT_DIR_OUT | 2, EP_TYPE_BULK, 64, 1);
}

void SendSensorData(void) {
	if (!device_configured)
	return;

	Endpoint_SelectEndpoint(ENDPOINT_DIR_IN | 1);
	
	if (Endpoint_IsINReady()) {
		Endpoint_Write_Stream_LE(&sensorData, sizeof(SensorData_t), NULL);
		Endpoint_ClearIN();
	}
}