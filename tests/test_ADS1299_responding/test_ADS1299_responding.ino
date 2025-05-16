#include <Arduino.h>
#include <SPI.h>

// --- Pin Mapping ---
static const uint8_t pin_MOSI_NUM = 23;
static const uint8_t pin_CS_NUM = 5;
static const uint8_t pin_MISO_NUM = 19;
static const uint8_t pin_SCK_NUM = 18; // this must change. Double check since there are two mappings rn
static const uint8_t pin_PWDN_NUM = 13;
static const uint8_t pin_RST_NUM = 12;
static const uint8_t pin_START_NUM = 14;
static const uint8_t pin_DRDY_NUM = 27;
static const uint8_t pin_LED_DEBUG = 17;

// --- SPI instance ---
SPIClass *vspi = NULL;  // SPI instance

// --- ADS1299 State Management ---
int _ADS1299_MODE = -2;
int ADS1299_MODE_SDATAC = 1;
int ADS1299_MODE_RDATAC = 2;

int _ADS1299_PREV_CMD = -1;
int CMD_ADC_WREG = 3;
int CMD_ADC_RREG = 4;
int CMD_ADC_SDATAC = 5;
int CMD_ADC_RDATAC = 6;
int CMD_ADC_START  = 7;

// --- Function Prototypes ---
void ADS1299_WREG(uint8_t regAdd, uint8_t* values, uint8_t numRegs);
void ADS1299_RREG(uint8_t regAdd, uint8_t* buffer, uint8_t numRegs);
void ADS1299_SETUP(void);
void ADS1299_SDATAC(void);
void ADS1299_RDATAC(void);
void ADS1299_START(void);
byte SPI_SendByte(byte data_byte, bool cont);
void read_ADS1299_data(byte *buffer);

// --- Register Setup ---
/* Data Structure for controlling registers */
typedef struct Deez {
    int add;
    int reg_val;
} regVal_pair;
int size_reg_ls = 22;
// Registers to setup. If -2, end WREG.
static const regVal_pair ADS1299_REGISTER_LS[22] = {
    {0x01, 0b10110000},
    {0x02, 0b11010000},
    {0x03, 0b11101100},
    {0x04, 0},
    {-2, -2},
    {0x05, 0b01100000},
    {0x06, 0b01100000},
    {0x07, 0b01100000},
    {0x08, 0b01100000},
    {0x09, 0b01100000},
    {0x0A, 0b01100000},
    {0x0B, 0b01100000},
    {0x0C, 0b01100000},
    {0x0D, 0b11111111},
    {0x0E, 0b11111111},
    {0x0F, 0},
    {0x10, 0},
    {0x11, 0},
    {-2, -2},
    {0x15, 0},
    {0x16, 0},
    {0x17, 0}
};

// --- SPI Send/Receive Byte (Arduino Framework) ---
byte SPI_SendByte(byte data_byte, bool cont) {
    if (!cont) {
        digitalWrite(pin_CS_NUM, LOW);  // Assert CS
    }

    byte received = vspi->transfer(data_byte); // Send and receive

//    if (data_byte == 0) {
 //       return -1;
  //  }

    if (!cont) {
        digitalWrite(pin_CS_NUM, HIGH); // De-assert CS
    }
    return received;
}

// --- ADS1299 Write Registers (Arduino) ---
void ADS1299_WREG(uint8_t regAdd, uint8_t* values, uint8_t numRegs) {
    if (_ADS1299_MODE != ADS1299_MODE_SDATAC) {
        ADS1299_SDATAC();
    }
    digitalWrite(pin_CS_NUM, LOW);
    SPI_SendByte(0x40 | (regAdd & 0x1F), true);
    SPI_SendByte(numRegs - 1, true);
    Serial.print("First register address: "); Serial.println(regAdd & 0x1F, HEX);
    Serial.print("Actual byte sent over: "); Serial.println(CMD_ADC_WREG | (regAdd & 0x1F), HEX);

    for (uint8_t i = 0; i < numRegs; i++) {
        SPI_SendByte(values[i], true);
        Serial.print("Value used: "); Serial.println(values[i], HEX);
    }
    digitalWrite(pin_CS_NUM, HIGH);
    _ADS1299_PREV_CMD = CMD_ADC_WREG;
}

// --- ADS1299 Read Registers (Arduino) ---
void ADS1299_RREG(uint8_t regAdd, uint8_t* buffer, uint8_t numRegs) {
    if (_ADS1299_MODE != ADS1299_MODE_SDATAC) {
        ADS1299_SDATAC();
    }
    digitalWrite(pin_CS_NUM, LOW);
    SPI_SendByte(0x20 | (regAdd & 0x1F), true);
    SPI_SendByte(numRegs - 1, true);
    for (uint8_t i = 0; i < numRegs; i++) {
        buffer[i] = SPI_SendByte(0x00, true); // Clock in data with dummy bytes
    }
    digitalWrite(pin_CS_NUM, HIGH);
    _ADS1299_PREV_CMD = CMD_ADC_RREG;
}

// --- ADS1299 SDATAC ---
void ADS1299_SDATAC(void) {
    SPI_SendByte(CMD_ADC_SDATAC, false);
    _ADS1299_MODE = ADS1299_MODE_SDATAC;
    _ADS1299_PREV_CMD = CMD_ADC_SDATAC;
    Serial.println("Sent SDATAC command!");
}

// --- ADS1299 RDATAC ---
void ADS1299_RDATAC(void) {
    SPI_SendByte(CMD_ADC_RDATAC, false);
    _ADS1299_MODE = ADS1299_MODE_RDATAC;
    _ADS1299_PREV_CMD = CMD_ADC_RDATAC;
    Serial.println("Send RDATAC command!");
}

// --- ADS1299 Start Command --
void ADS1299_START(void) {
  SPI_SendByte(CMD_ADC_START, false);
  _ADS1299_PREV_CMD = CMD_ADC_START;
}

// --- ADS1299 Setup (Arduino) ---
void ADS1299_SETUP(void) {
    digitalWrite(pin_PWDN_NUM, LOW);
    digitalWrite(pin_RST_NUM, LOW);
    Serial.println("Init pins low");
    delay(100);

    digitalWrite(pin_PWDN_NUM, HIGH);
    digitalWrite(pin_RST_NUM, HIGH);
    Serial.println("Init pins high");
    delay(1000);

    ADS1299_SDATAC();

    uint8_t refbuf[] = {0b11101100}; //0b11100000 IS OLD SETTING
    uint8_t value[1]; //register setting placeholder var
    ADS1299_WREG(0x03, refbuf, 1);
    delay(10);

    uint8_t i = 0;
    while (i < size_reg_ls) {
        const regVal_pair temp = ADS1299_REGISTER_LS[i];
        if (temp.add == -2) { // stop transaction and start new one
            i++;
            continue;
        }
        value[0] = {(uint8_t)temp.reg_val};
        ADS1299_WREG(temp.add, value, 1);
        delay(1); // Short delay between register writes
        i++;
    }
}

/* Read ADS1299 Data 
   Accepts Byte Array */
void read_ADS1299_data(byte *buffer) {
    digitalWrite(pin_CS_NUM, LOW);
    for (int i = 0; i < 27; i++) { // 3 status bytes + 8 channels * 3 bytes/channel
        buffer[i] = SPI_SendByte(0x00, true);
    }
    digitalWrite(pin_CS_NUM, HIGH);
}

// SETUP FUNCTION
void setup() {
  // put your setup code here, to run once:
  // --- Serial Initialization ---
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for serial port to connect
    }

    // --- GPIO Configuration ---
    pinMode(pin_PWDN_NUM, OUTPUT);
    pinMode(pin_RST_NUM, OUTPUT);
    pinMode(pin_START_NUM, OUTPUT);
    pinMode(pin_CS_NUM, OUTPUT);
    pinMode(pin_DRDY_NUM, INPUT_PULLUP); // Use internal pull-up
    pinMode(pin_LED_DEBUG, OUTPUT);
    digitalWrite(pin_CS_NUM, HIGH);  // Initialize CS high
    delay(2000);

    // --- SPI Initialization ---
    // Check if we're using ESP32 or AVR
    #if defined(ESP32)
        // ESP32 SPI initialization
        vspi = new SPIClass(VSPI);  // Create VSPI instance
        vspi->begin(pin_SCK_NUM, pin_MISO_NUM, pin_MOSI_NUM, pin_CS_NUM); // Initialize SPI
        vspi->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); // 4 MHz SPI clock, MSB first, Mode 0
    #else
        // Standard Arduino SPI initialization
        //vspi = &SPI;  // Use the default SPI instance
        vspi->begin(pin_SCK_NUM, pin_MISO_NUM, pin_MOSI_NUM, pin_CS_NUM);
        vspi->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); // 4 MHz SPI clock, MSB first, Mode 0
    #endif

    // --- ADS1299 Initialization ---
    ADS1299_SETUP();

    Serial.println("Setup complete.");
    digitalWrite(pin_START_NUM, HIGH);
    ADS1299_RDATAC();
    delay(100);
}

// LOOP FUNCTION
void loop() {
  if (digitalRead(pin_DRDY_NUM) == LOW) {
    byte raw_data[27];
    read_ADS1299_data(raw_data);

    // Create the data packet with timestamp
    unsigned long timestamp = millis();
    Serial.write((uint8_t *)&timestamp, sizeof(timestamp)); // Send 4-byte timestamp
    Serial.write(raw_data, 27); // Send 27 bytes of ADS1299 data
    /*for (int i=0; i<27; i++) {
        Serial.println(raw_data[i]);
    }*/
  }
  delay(1); // Small delay to avoid overwhelming the serial port
}