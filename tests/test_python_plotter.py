import struct
import serial
import csv
import time
from collections import deque

import numpy as np
import matplotlib.pyplot as plt

# --- Configuration ---
SERIAL_PORT = 'COM4'  # Replace with your Arduino's serial port
BAUD_RATE = 115200
NUM_CHANNELS = 8
BYTES_PER_CHANNEL = 3
STATUS_BYTES = 3
PACKET_SIZE = 4 + STATUS_BYTES + (NUM_CHANNELS * BYTES_PER_CHANNEL)  # Timestamp + Status + 8 channels
CSV_FILENAME = 'ads1299_data.csv'
SMOOTHING_WINDOW = 5  # Number of data points to average for smoothing

# --- Data Buffers for Plotting ---
DISPLAY_TIME_WINDOW = 5  # Seconds of data to display
SAMPLE_RATE = 115200  # Approximate sample rate (adjust as needed)
MAX_SAMPLES = int(DISPLAY_TIME_WINDOW * SAMPLE_RATE)
raw_data_buffers = [deque(maxlen=MAX_SAMPLES) for _ in range(NUM_CHANNELS)]  # Store raw voltage values
smoothed_data_buffers = [deque(maxlen=MAX_SAMPLES) for _ in range(NUM_CHANNELS)] # Store smoothed values
time_buffer = deque(maxlen=MAX_SAMPLES)
start_time = time.time()

# --- Setup Plot ---
fig, axs = plt.subplots(NUM_CHANNELS, 1, sharex=True)
fig.suptitle('Smoothed ADS1299 Channel Data')
lines = [ax.plot([], [])[0] for ax in axs]

plt.ion()  # Turn on interactive mode for live plotting
plt.show()

# --- CSV File Handling ---
csvfile = open(CSV_FILENAME, 'w', newline='')
csv_writer = csv.writer(csvfile)
csv_writer.writerow(['Timestamp (s)'] + [f'Channel {i+1} (V)' for i in range(NUM_CHANNELS)])

def moving_average(data, window_size):
    if not data:
        return []
    window = deque(maxlen=window_size)
    smoothed_data = []
    for item in data:
        window.append(item)
        smoothed_data.append(sum(window) / len(window))
    return np.array(smoothed_data)

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud. Writing data to {CSV_FILENAME}")

    while True:
        if ser.in_waiting >= PACKET_SIZE:
            raw_packet = ser.read(PACKET_SIZE)  # Sends in the 31 bytes at a time
            #print("--")
            #print(raw_packet)

            # Unpack timestamp
            timestamp_ms = struct.unpack('<L', raw_packet[:4])[0]
            #timestamp_s = timestamp_ms / 1000.0  # Convert ms to seconds

            # Unpack ADS1299 data
            status_bytes = raw_packet[4: 4 + STATUS_BYTES]
            channel_data_bytes = raw_packet[4 + STATUS_BYTES:]
            voltage_values = []

            for i in range(NUM_CHANNELS):
                start_index = i * BYTES_PER_CHANNEL
                end_index = start_index + BYTES_PER_CHANNEL
                channel_bytes = channel_data_bytes[start_index:end_index]

                # Convert 3 bytes to a 24-bit signed integer (MSB first) and then to voltage
                if len(channel_bytes) == 3:
                    value = struct.unpack('>i', b'\x00' + channel_bytes)[0] >> 8
                    voltage = value * 2.5 / (2**23)
                    raw_data_buffers[i].append(voltage)
                    voltage_values.append(voltage)
                else:
                    voltage_values.append(None) # Handle incomplete data if needed

            # Write data to CSV file (raw voltage values)
            csv_writer.writerow([timestamp_ms] + voltage_values)

            current_time = time.time() - start_time
            time_buffer.append(current_time)

            # Apply smoothing
            for i in range(NUM_CHANNELS):
                smoothed_data = moving_average(list(raw_data_buffers[i]), SMOOTHING_WINDOW)
                if smoothed_data.any():
                    smoothed_data_buffers[i].append(smoothed_data[-1]) # Only keep the latest smoothed value

            # Update the plots
            current_time_window = time_buffer[-1] if time_buffer else 0
            start_time_window = current_time_window - DISPLAY_TIME_WINDOW

            for i, ax in enumerate(axs):
                ax.clear()
                relevant_time = []
                relevant_smoothed_data = []

                current_time_window = time_buffer[-1] if time_buffer else 0
                start_time_window = current_time_window - DISPLAY_TIME_WINDOW

                # Filter time and smoothed data within the display window
                for t, data_point in zip(time_buffer, smoothed_data_buffers[i]):
                    if start_time_window <= t <= current_time_window:
                        relevant_time.append(t)
                        relevant_smoothed_data.append(data_point)

                if relevant_time and relevant_smoothed_data and len(relevant_time) == len(relevant_smoothed_data):
                    ax.plot(relevant_time, relevant_smoothed_data)

                ax.set_ylabel(f'Channel {i+1} (V)')
                ax.set_xlim(start_time_window, current_time_window)
                #ax.set_ylim(-0.001, 0.001)

            axs[-1].set_xlabel('Time (s)')
            fig.canvas.draw()
            fig.canvas.flush_events()

except serial.SerialException as e:
    print(f"Error communicating with serial port {SERIAL_PORT}: {e}")
except KeyboardInterrupt:
    print("Exiting...")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    csvfile.close()
    print(f"Data written to {CSV_FILENAME}")