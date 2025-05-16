import struct
import serial
import numpy as np
import time
from collections import deque
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt

# Necessary Global Variables
global SERIAL_PORT, SERIAL_OBJECT, SERIAL_PORT_BAUD, SERIAL_MSG_SIZE, EEG_CHANNEL_BYTES, EEG_CHANNEL_NUM, STATUS_BYTES, TIMESTAMP_BYTES, GRAPH_TIME_WINDOW, GRAPH_UPDATE_INTERVAL, COL_COUNT, CURRENT_TIME_STEP, T_DATA, EEG_DATA

SERIAL_PORT = "COM5"
SERIAL_PORT_BAUD = 115200
EEG_CHANNEL_NUM = 8
EEG_CHANNEL_BYTES = 3
STATUS_BYTES = 3
TIMESTAMP_BYTES = 4
SERIAL_MSG_SIZE = TIMESTAMP_BYTES + STATUS_BYTES + EEG_CHANNEL_NUM * EEG_CHANNEL_BYTES
GRAPH_TIME_WINDOW = 4 # seconds
GRAPH_UPDATE_INTERVAL = 100 # ms
COL_COUNT = 0
T_DATA = []
EEG_DATA = [[] for _ in range(8)]

def init():
    global CURRENT_TIME_STEP, T_DATA, EEG_DATA
    CURRENT_TIME_STEP = 0
    T_DATA = []
    EEG_DATA = [[] for _ in range(8)]

    for i, line in enumerate(lines):
        line.set_data([], [])
        axes_flat[i].set_xlim(0, 100000)

    return lines

def update(ser, lines, axes_flat, temp):
    # This is to read it in via Serial port
    if ser.in_waiting >= SERIAL_MSG_SIZE:
        raw_packet = ser.read(SERIAL_MSG_SIZE)
        timestamp_ms = struct.unpack('<L', raw_packet[:4])[0]
        status_bytes = raw_packet[4: 4+STATUS_BYTES]
        channel_data_bytes = raw_packet[4+STATUS_BYTES: ]
        temp.append(timestamp_ms)
        T_DATA.append(timestamp_ms)

        # PARSE EEG DATA NOW BABE
        for i in range(EEG_CHANNEL_NUM):
            this_channel = channel_data_bytes[i*EEG_CHANNEL_BYTES: i*EEG_CHANNEL_BYTES+EEG_CHANNEL_BYTES]

            if len(this_channel) == 3:
                value = struct.unpack('>i', b'\x00' + this_channel)[0] >> 8 # Still don't understand this, might be causing more issues
                voltage = value * 2.5 * 1000000 / (2**23) # Convert to microvolts
                temp.append(voltage) # Now temp is filled up like a chocolate bun waiting to be delivered to a deli to be consumed by me and Chandiran
                EEG_DATA[i].append(voltage)
            else:
                pass

        # HEY BABE UPDATE THE DATA IN LINES
        for i, line in enumerate(lines):
            current_t_data = T_DATA[-10000: ]
            current_EEG_data = EEG_DATA[i][-10000:]
            line.set_data(current_t_data, current_EEG_data)

            # Get scrolling effect
            if len(current_t_data) > 10000:
                xmin = current_t_data[0]
                xmax = current_t_data[-1]
                axes_flat[i].set_xlim(xmin, xmax)

            else:
                axes_flat[i].set_xlim(0, 10000)

            # Re-set y-axis limits
            axes_flat[i].relim()
            axes_flat[i].autoscale_view(scalex=False, scaley=True)

    return lines

# Setup Baby
def setup():
    # Serial Comms Object
    global SERIAL_OBJECT
    SERIAL_OBJECT = serial.Serial(SERIAL_PORT, SERIAL_PORT_BAUD)
    print(f"Connected to {SERIAL_PORT} at {SERIAL_PORT_BAUD} baud.")
    
    # Graph Objects
    # This is to set up the graph
    fig, axes =  plt.subplots(8, 1, figsize=(15,7))
    axes_flat = axes.flatten()

    # Creating line objects for each subplot
    lines = []
    for i in range(8):
        line, = axes_flat[i].plot([], [], lw=2)
        lines.append(line)
        axes_flat[i].set_title(f'Sensor {i+1}')
        axes_flat[i].set_ylim(-1.5, 1.5)
        axes_flat[i].set_xlim(0, 10000)

    # Initialize values into these lines of data
    for i, line in enumerate(lines):
        line.set_data([], []) # Now each entry in lines list has init data in it
        axes_flat[i].set_xlim(0, 10000)

    return [SERIAL_OBJECT, lines, axes_flat, fig]

# Example function to parse your data format
def parse_eeg_format(data_line):
    """
    Parse a line of data in the format:
    timestamp x y z ...
    or
    EEGn x y z ...
    
    Returns the values as a list.
    """
    values = data_line.strip().split()
    return values[1:]  # Return all values except the first (which is the label)

# Setting Up Matrix that picks up the data
final_data = np.transpose(np.matrix(["Timestamp", "EEG1", "EEG2", "EEG3", "EEG4", "EEG5", "EEG6", "EEG7", "EEG8"]))

if __name__ == "__main__":
    ser, lines, axes_flat, fig = setup()
    temp = []
    ani = FuncAnimation(fig, update(ser, lines, axes_flat, temp), frames=None, interval=GRAPH_UPDATE_INTERVAL, blit=False)
    plt.tight_layout(pad=2.0)
    plt.show()