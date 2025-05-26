#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <stdint.h>
#include <zephyr/kernel.h>

// ADS1299 data constants
#define ADS1299_NUM_CHANNELS    8
#define ADS1299_BYTES_PER_CHANNEL   3  // 24-bit data
#define ADS1299_STATUS_BYTES    3      // 24-bit status
#define ADS1299_TOTAL_DATA_BYTES    (ADS1299_STATUS_BYTES + (ADS1299_NUM_CHANNELS * ADS1299_BYTES_PER_CHANNEL))

// Packet format constants
#define PACKET_HEADER_SIZE      4
#define PACKET_TIMESTAMP_SIZE   8
#define PACKET_CRC_SIZE         2
#define PACKET_TRAILER_SIZE     2
#define PACKET_OVERHEAD         (PACKET_HEADER_SIZE + PACKET_TIMESTAMP_SIZE + PACKET_CRC_SIZE + PACKET_TRAILER_SIZE)

// Packet identifiers
#define PACKET_START_BYTE1      0xAA
#define PACKET_START_BYTE2      0x55
#define PACKET_TYPE_ADS1299     0x01
#define PACKET_END_BYTE1        0x55
#define PACKET_END_BYTE2        0xAA

// Data structures
typedef struct {
    uint32_t timestamp_us;      // Microsecond timestamp
    uint32_t sample_number;     // Incremental sample counter
    uint32_t status;            // 24-bit status register
    int32_t channels[ADS1299_NUM_CHANNELS]; // Channel data (sign-extended from 24-bit)
    uint8_t lead_off_status_p;  // Lead-off status positive
    uint8_t lead_off_status_n;  // Lead-off status negative
    uint8_t gpio_status;        // GPIO status
} ads1299_sample_t;

typedef struct {
    uint8_t start_bytes[2];     // 0xAA, 0x55
    uint8_t packet_type;        // 0x01 for ADS1299
    uint8_t payload_length;     // Length of payload
    uint64_t timestamp_us;      // 8-byte timestamp
    ads1299_sample_t sample;    // ADS1299 sample data
    uint16_t crc16;             // CRC-16 checksum
    uint8_t end_bytes[2];       // 0x55, 0xAA
} __attribute__((packed)) ads1299_packet_t;

// Function declarations
void process_ads1299_data(const uint8_t *raw_data, ads1299_sample_t *sample);
size_t format_sample_for_transmission(const ads1299_sample_t *sample, 
                                      uint8_t *buffer, size_t buffer_size);
uint16_t calculate_crc16(const uint8_t *data, size_t length);
int32_t convert_24bit_to_32bit(const uint8_t *data);
uint64_t get_timestamp_us(void);

// Data integrity functions
bool validate_packet(const ads1299_packet_t *packet);
void init_data_handler(void);

#endif // DATA_HANDLER_H