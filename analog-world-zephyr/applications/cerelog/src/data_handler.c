#include "data_handler.h"
#include <zephyr/sys/printk.h>
#include <zephyr/timing/timing.h>
#include <string.h>

// Static variables
static uint32_t sample_counter = 0;
static timing_t start_time;

// CRC-16 lookup table (CRC-16-CCITT)
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

void init_data_handler(void) {
    timing_init();
    start_time = timing_counter_get();
    sample_counter = 0;
    printk("Data handler initialized\n");
}

uint64_t get_timestamp_us(void) {
    timing_t current_time = timing_counter_get();
    uint64_t cycles = timing_cycles_get(&start_time, &current_time);
    return timing_cycles_to_ns(cycles) / 1000; // Convert to microseconds
}

uint16_t calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc = (crc << 8) ^ crc16_table[((crc >> 8) ^ data[i]) & 0xFF];
    }
    
    return crc;
}

int32_t convert_24bit_to_32bit(const uint8_t *data) {
    // Convert 24-bit two's complement to 32-bit signed integer
    int32_t result = (data[0] << 24) | (data[1] << 16) | (data[2] << 8);
    return result >> 8; // Sign extend by shifting right
}

void process_ads1299_data(const uint8_t *raw_data, ads1299_sample_t *sample) {
    if (!raw_data || !sample) {
        printk("Invalid parameters in process_ads1299_data\n");
        return;
    }
    
    // Get timestamp first
    sample->timestamp_us = (uint32_t)get_timestamp_us();
    sample->sample_number = ++sample_counter;
    
    // Extract 24-bit status (first 3 bytes)
    sample->status = (raw_data[0] << 16) | (raw_data[1] << 8) | raw_data[2];
    
    // Parse status bits according to ADS1299 datasheet
    // Status format: 1100 + LOFF_STATP + LOFF_STATN + GPIO[4:7]
    sample->lead_off_status_p = (raw_data[1] >> 4) & 0x0F; // LOFF_STATP
    sample->lead_off_status_n = raw_data[1] & 0x0F;        // LOFF_STATN
    sample->gpio_status = raw_data[2] & 0x0F;              // GPIO[4:7]
    
    // Extract channel data (24-bit each, starting from byte 3)
    for (int ch = 0; ch < ADS1299_NUM_CHANNELS; ch++) {
        int offset = ADS1299_STATUS_BYTES + (ch * ADS1299_BYTES_PER_CHANNEL);
        sample->channels[ch] = convert_24bit_to_32bit(&raw_data[offset]);
    }
}

size_t format_sample_for_transmission(const ads1299_sample_t *sample, uint8_t *buffer, size_t buffer_size) {
    if (!sample || !buffer) {
        return 0;
    }
    
    // Calculate required buffer size
    size_t required_size = sizeof(ads1299_packet_t);
    if (buffer_size < required_size) {
        printk("Buffer too small: need %zu, have %zu\n", required_size, buffer_size);
        return 0;
    }
    
    ads1299_packet_t *packet = (ads1299_packet_t *)buffer;
    
    // Fill packet header
    packet->start_bytes[0] = PACKET_START_BYTE1;
    packet->start_bytes[1] = PACKET_START_BYTE2;
    packet->packet_type = PACKET_TYPE_ADS1299;
    packet->payload_length = sizeof(ads1299_sample_t) + sizeof(uint64_t); // sample + timestamp
    
    // Add timestamp
    packet->timestamp_us = get_timestamp_us();
    
    // Copy sample data
    memcpy(&packet->sample, sample, sizeof(ads1299_sample_t));
    
    // Calculate CRC over header + timestamp + sample data
    size_t crc_data_size = PACKET_HEADER_SIZE + PACKET_TIMESTAMP_SIZE + sizeof(ads1299_sample_t);
    packet->crc16 = calculate_crc16((uint8_t *)packet, crc_data_size);
    
    // Fill packet trailer
    packet->end_bytes[0] = PACKET_END_BYTE1;
    packet->end_bytes[1] = PACKET_END_BYTE2;
    
    return required_size;
}

bool validate_packet(const ads1299_packet_t *packet) {
    if (!packet) {
        return false;
    }
    
    // Check start bytes
    if (packet->start_bytes[0] != PACKET_START_BYTE1 || 
        packet->start_bytes[1] != PACKET_START_BYTE2) {
        return false;
    }
    
    // Check end bytes
    if (packet->end_bytes[0] != PACKET_END_BYTE1 || 
        packet->end_bytes[1] != PACKET_END_BYTE2) {
        return false;
    }
    
    // Check packet type
    if (packet->packet_type != PACKET_TYPE_ADS1299) {
        return false;
    }
    
    // Verify CRC
    size_t crc_data_size = PACKET_HEADER_SIZE + PACKET_TIMESTAMP_SIZE + sizeof(ads1299_sample_t);
    uint16_t calculated_crc = calculate_crc16((uint8_t *)packet, crc_data_size);
    
    if (calculated_crc != packet->crc16) {
        printk("CRC mismatch: calculated 0x%04x, packet 0x%04x\n", 
               calculated_crc, packet->crc16);
        return false;
    }
    
    return true;
}

// Debug function to print sample data
void print_sample_debug(const ads1299_sample_t *sample) {
    if (!sample) {
        return;
    }
    
    printk("Sample #%u @ %u us:\n", sample->sample_number, sample->timestamp_us);
    printk("  Status: 0x%06x, LOFF_P: 0x%02x, LOFF_N: 0x%02x, GPIO: 0x%02x\n", 
           sample->status, sample->lead_off_status_p, 
           sample->lead_off_status_n, sample->gpio_status);
    
    for (int ch = 0; ch < ADS1299_NUM_CHANNELS; ch++) {
        printk("  CH%d: %d (0x%08x)\n", ch + 1, sample->channels[ch], 
               (uint32_t)sample->channels[ch]);
    }
}