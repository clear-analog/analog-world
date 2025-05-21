import struct
import serial
from PyQt5 import QtWidgets, QtCore
from pyqtgraph import PlotWidget, plot
import time
from collections import deque

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec
from brainflow.data_filter import DataFilter, FilterTypes, WindowOperations, DetrendOperations

# --- EEG Frequency Bands (Hz) ---
FREQ_BANDS = {
    'Delta': (0.5, 4),
    'Theta': (4, 8),
    'Alpha': (8, 13),
    'Beta': (13, 30),
    'Gamma': (30, 50)
}

# --- Configuration ---
SERIAL_PORT = 'COM5'
BAUD_RATE = 115200
NUM_CHANNELS = 8
BYTES_PER_CHANNEL = 3
STATUS_BYTES = 3
PACKET_SIZE = 4 + STATUS_BYTES + (NUM_CHANNELS * BYTES_PER_CHANNEL)
CSV_FILENAME = 'gtesttest.csv'
SAMPLE_RATE = 250  # BrainFlow needs a realistic EEG sample rate; ADS1299 often runs at 250–500 Hz
final_data = np.zeros((9,1))

# Filter settings
FILTER_ORDER = 4  # Butterworth filter order (4 is typically good for EEG)
NOTCH_FREQ = 60.0  # For power line noise (use 50.0 for countries with 50Hz power)
NOTCH_Q = 30.0  # Q-factor for notch filter (higher means narrower)
ENABLE_NOTCH = True  # Set to False if your signals are already clean

# --- Data Buffers ---
DISPLAY_TIME_WINDOW = 5  # Seconds
MAX_SAMPLES = int(DISPLAY_TIME_WINDOW * SAMPLE_RATE)
BUFFER_SIZE = max(MAX_SAMPLES, SAMPLE_RATE * 4)  # At least 4 seconds of data for good filtering

# Raw and filtered data buffers
raw_data_buffers = [deque(maxlen=BUFFER_SIZE) for _ in range(NUM_CHANNELS)]
filtered_bands = {band: [deque(maxlen=BUFFER_SIZE) for _ in range(NUM_CHANNELS)] for band in FREQ_BANDS}
time_buffer = deque(maxlen=BUFFER_SIZE)
start_time = time.time()
last_plot_time = start_time
col_count = 0
count = 0
final_data = np.transpose(np.matrix(["Timestamp", "EEG1", "EEG2", "EEG3", "EEG4", "EEG5", "EEG6", "EEG7", "EEG8"]))

# --- Setup Plot ---
plt.ion()  # Interactive mode
fig = plt.figure(figsize=(15, 10))
gs = GridSpec(len(FREQ_BANDS) + 1, 1, figure=fig)

# Raw data plot (top)
raw_ax = fig.add_subplot(gs[0])
raw_ax.set_title('Raw EEG Signal')
raw_ax.set_ylabel('Voltage (V)')
raw_lines = [raw_ax.plot([], [], label=f'Ch {i+1}')[0] for i in range(NUM_CHANNELS)]
raw_ax.legend(loc='upper right')

# Filtered band plots (one per band)
band_axes = []
band_lines = []
for i, band in enumerate(FREQ_BANDS.keys(), 1):
    low_freq, high_freq = FREQ_BANDS[band]
    ax = fig.add_subplot(gs[i])
    ax.set_title(f'{band} Band ({low_freq}–{high_freq} Hz)')
    ax.set_ylabel('Amplitude')
    lines = [ax.plot([], [], label=f'Ch {i+1}')[0] for i in range(NUM_CHANNELS)]
    band_axes.append(ax)
    band_lines.append(lines)

# Bottom plot x-axis label
band_axes[-1].set_xlabel('Time (s)')

plt.tight_layout()
plt.show()

# --- CSV File ---
#csvfile = open(CSV_FILENAME, 'w', newline='')
#scsv_writer = csv.writer(csvfile)

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud. Writing data to {CSV_FILENAME}")
    col_count = col_count + 1

    while count < 100:
        temp = []

        if ser.in_waiting >= PACKET_SIZE:
            raw_packet = ser.read(PACKET_SIZE)
            timestamp_ms = struct.unpack('<L', raw_packet[:4])[0]
            status_bytes = raw_packet[4: 4 + STATUS_BYTES]
            channel_data_bytes = raw_packet[4 + STATUS_BYTES:]
            current_time = time.time() - start_time
            time_buffer.append(current_time)
            temp.append(timestamp_ms)

            # Collect 8 channels worth of data
            for i in range(NUM_CHANNELS):
                start_index = i * BYTES_PER_CHANNEL
                end_index = start_index + BYTES_PER_CHANNEL
                channel_bytes = channel_data_bytes[start_index:end_index]

                if len(channel_bytes) == 3:
                    value = struct.unpack('>i', b'\x00' + channel_bytes)[0] >> 8 # This will append the three bytes next to each other to obtain the actual number being hidden
                    voltage = value * 2.5 / (2**23) * 1000000 # Convert to microvolts
                    raw_data_buffers[i].append(voltage)
                    temp.append(voltage)
                else:
                    pass

            # Package the data now
            final_data = np.concatenate((final_data, np.concatenate(np.transpose(np.matrix(temp)))), axis=1)
            
            # Refresh the plots
            fig.canvas.draw()
            fig.canvas.flush_events()

        col_count = col_count + 1
        count = count + 1


except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("Exiting...")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    #print(f"Data written to {CSV_FILENAME}")