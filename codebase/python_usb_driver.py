import usb.core
import usb.util
import struct
import time

class USBDriver:
    def __init__(self, vendor_id=0x0483, product_id=0x5740):  # Default IDs, adjust as needed
        self.vendor_id = vendor_id
        self.product_id = product_id
        self.device = None
        self.endpoint_in = None
        self.endpoint_out = None
        self.packet_size = 64  # Standard USB packet size

    def connect(self):
        """Establish connection with the USB device"""
        try:
            # Find the device
            self.device = usb.core.find(idVendor=self.vendor_id, idProduct=self.product_id)
            
            if self.device is None:
                raise ValueError('Device not found')

            # Set the active configuration
            self.device.set_configuration()

            # Get an endpoint instance
            cfg = self.device.get_active_configuration()
            intf = cfg[(0,0)]

            # Find the input endpoint
            self.endpoint_in = usb.util.find_descriptor(
                intf,
                custom_match=lambda e: 
                    usb.util.endpoint_direction(e.bEndpointAddress) == 
                    usb.util.ENDPOINT_IN
            )

            # Find the output endpoint
            self.endpoint_out = usb.util.find_descriptor(
                intf,
                custom_match=lambda e: 
                    usb.util.endpoint_direction(e.bEndpointAddress) == 
                    usb.util.ENDPOINT_OUT
            )

            if self.endpoint_in is None or self.endpoint_out is None:
                raise ValueError('Endpoints not found')

            return True

        except Exception as e:
            print(f"Connection error: {str(e)}")
            return False

    def read_data(self, timeout=1000):
        """Read data from the USB device"""
        try:
            data = self.endpoint_in.read(self.packet_size, timeout)
            return bytes(data)
        except usb.core.USBError as e:
            if e.args[0] == 110:  # Timeout error
                return None
            else:
                raise

    def write_data(self, data, timeout=1000):
        """Write data to the USB device"""
        try:
            self.endpoint_out.write(data, timeout)
            return True
        except usb.core.USBError as e:
            print(f"Write error: {str(e)}")
            return False

    def disconnect(self):
        """Disconnect from the USB device"""
        if self.device:
            usb.util.dispose_resources(self.device)

def main():
    # Example usage
    driver = USBDriver()
    
    if driver.connect():
        print("USB device connected successfully")
        
        try:
            while True:
                # Read data
                data = driver.read_data()
                if data:
                    print(f"Received data: {data}")
                
                time.sleep(0.1)  # Small delay to prevent CPU overuse
                
        except KeyboardInterrupt:
            print("\nStopping USB communication...")
        finally:
            driver.disconnect()
            print("USB device disconnected")
    else:
        print("Failed to connect to USB device")

if __name__ == "__main__":
    main()