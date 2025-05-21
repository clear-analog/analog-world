import struct
import serial
import numpy as np
import time
from collections import deque # Consider using deque for T_DATA and EEG_DATA
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt

# Necessary Global Variables (consider reducing global scope later)
global SERIAL_PORT, SERIAL_OBJECT, SERIAL_PORT_BAUD, SERIAL_MSG_SIZE, EEG_CHANNEL_BYTES, EEG_CHANNEL_NUM, STATUS_BYTES, TIMESTAMP_BYTES, GRAPH_TIME_WINDOW, GRAPH_UPDATE_INTERVAL, COL_COUNT, CURRENT_TIME_STEP, T_DATA, EEG_DATA

SERIAL_PORT = "COM4"
SERIAL_PORT_BAUD = 115200
EEG_CHANNEL_NUM = 8
EEG_CHANNEL_BYTES = 3
STATUS_BYTES = 3
TIMESTAMP_BYTES = 4
SERIAL_MSG_SIZE = TIMESTAMP_BYTES + STATUS_BYTES + EEG_CHANNEL_NUM * EEG_CHANNEL_BYTES
GRAPH_TIME_WINDOW = 4  # seconds
GRAPH_UPDATE_INTERVAL = 100  # ms
SERIAL_OBJECT = serial.Serial(SERIAL_PORT, SERIAL_PORT_BAUD)

# COL_COUNT = 0 # Unused global variable
T_DATA = []
EEG_DATA = [[] for _ in range(EEG_CHANNEL_NUM)] # Used EEG_CHANNEL_NUM

# This init function is not currently used by FuncAnimation.
# If you want to use it, pass it to FuncAnimation's init_func parameter.
# def init_animation_func(): # Renamed to avoid conflict if you make it a proper init_func
#     global CURRENT_TIME_STEP, T_DATA, EEG_DATA
#     # CURRENT_TIME_STEP = 0 # Unused global variable
#     T_DATA = []
#     EEG_DATA = [[] for _ in range(EEG_CHANNEL_NUM)]
#
#     # This part needs 'lines' and 'axes_flat' to be accessible
#     # For example, by defining this function within the main scope or making lines/axes_flat global
#     # for i, line in enumerate(lines):
#     #     line.set_data([], [])
#     #     axes_flat[i].set_xlim(0, GRAPH_TIME_WINDOW * 1000) # Consistent Xlim
#     # return lines


# Corrected update function signature
def update(frame, ser, lines, axes_flat, temp): # Added 'frame' as the first argument
    if ser.in_waiting >= SERIAL_MSG_SIZE:
        raw_packet = ser.read(SERIAL_MSG_SIZE)
        timestamp_ms = struct.unpack('<L', raw_packet[:4])[0]

        status_bytes = raw_packet[4: 4+STATUS_BYTES] # status_bytes is read but not used
        channel_data_bytes = raw_packet[4+STATUS_BYTES: ]
        
        temp.append(timestamp_ms) # 'temp' list accumulates data but isn't used elsewhere for plotting
        T_DATA.append(timestamp_ms)

        for i in range(EEG_CHANNEL_NUM):
            this_channel = channel_data_bytes[i*EEG_CHANNEL_BYTES : i*EEG_CHANNEL_BYTES+EEG_CHANNEL_BYTES]

            if len(this_channel) == 3:
                # Consider using int.from_bytes for 24-bit conversion:
                # value = int.from_bytes(this_channel, byteorder='big', signed=True)
                value = struct.unpack('>i', b'\x00' + this_channel)[0] >> 8
                voltage = value * 2.5 * 1000000 / (2**23)
                temp.append(voltage)
                EEG_DATA[i].append(voltage)
            # else: # If len(this_channel) != 3, this channel's data for this frame will be missing.
                  # Consider appending a placeholder like np.nan or handling this case.
                  # EEG_DATA[i].append(np.nan) # Example placeholder

        # Update the data in lines
        for i, line in enumerate(lines):
            # Plot up to the last 10000 points.
            # Consider using collections.deque(maxlen=...) for T_DATA and EEG_DATA
            # to automatically manage their size.
            current_t_data = T_DATA[-10000:]
            current_EEG_data = EEG_DATA[i][-10000:]
            line.set_data(current_t_data, current_EEG_data)

            # X-axis scrolling logic:
            # This ensures the x-axis shows a window of GRAPH_TIME_WINDOW seconds
            if T_DATA: # Check if T_DATA is not empty
                last_time_ms = T_DATA[-1]
                #window_width_ms = GRAPH_TIME_WINDOW * 1000

                # Determine xmin and xmax for the plot window
                # This logic makes the plot scroll once data exceeds the initial window.
                # Assumes timestamps start near 0 or that initial plot range handles the actual start time.
                current_plot_xmin, current_plot_xmax = axes_flat[i].get_xlim()
                
                if last_time_ms > current_plot_xmax: # If data has moved beyond the current window's right edge
                    # Scroll the window
                    new_xmax = last_time_ms
                    #new_xmin = new_xmax - window_width_ms
                    #axes_flat[i].set_xlim(new_xmin, new_xmax)
                # If data is still within the initial fixed window [initial_xmin, initial_xmin + window_width_ms],
                # and we haven't accumulated enough time to scroll past it,
                # no xlim change is needed if setup correctly initialized it.
                # The initial xlim is set in setup().

            # Re-set y-axis limits
            #axes_flat[i].relim()
            axes_flat[i].autoscale_view(scalex=True, scaley=True)
    else:
        print('SEXY TIMEs')
    return lines # FuncAnimation needs an iterable of artists that were updated

def setup():
    print(f"Connected to {SERIAL_PORT} at {SERIAL_PORT_BAUD} baud.")
    
    fig, axes =  plt.subplots(EEG_CHANNEL_NUM, 1, figsize=(15,7)) # Used EEG_CHANNEL_NUM
    axes_flat = axes.flatten()

    lines = []
    initial_xlim_max = GRAPH_TIME_WINDOW * 1000 # e.g., 4 seconds * 1000 ms/sec = 4000 ms
    for i in range(EEG_CHANNEL_NUM): # Used EEG_CHANNEL_NUM
        line, = axes_flat[i].plot([], [], lw=2)
        lines.append(line)
        axes_flat[i].set_title(f'Sensor {i+1}')
        axes_flat[i].set_ylim(-1.5, 1.5) # Initial Y limits
        # Set initial X limits based on GRAPH_TIME_WINDOW. Assumes timestamps start near 0.
        # If timestamps can start at arbitrary values, this might need adjustment
        # e.g. by getting an initial timestamp and setting xlim relative to it.
        axes_flat[i].set_xlim(0, initial_xlim_max)

    # The following loop seems redundant as lines are already empty and xlims are set.
    # for i, line in enumerate(lines):
    #     line.set_data([], []) 
    #     axes_flat[i].set_xlim(0, initial_xlim_max)

    return [SERIAL_OBJECT, lines, axes_flat, fig]

# `parse_eeg_format` is defined but not used. Consider removing if not needed.
# def parse_eeg_format(data_line):
#     values = data_line.strip().split()
#     return values[1:]

# `final_data` is defined but not used. Consider removing if not needed.
# final_data = np.transpose(np.matrix(["Timestamp", "EEG1", "EEG2", "EEG3", "EEG4", "EEG5", "EEG6", "EEG7", "EEG8"]))

if __name__ == "__main__":
    ser, lines, axes_flat, fig = setup()
    temp = [] # This list is populated but not used for plotting. Consider its purpose.

    # Corrected FuncAnimation call:
    # Pass 'update' function itself, not 'update(...)'
    # Pass additional arguments via fargs
    # Added cache_frame_data=False to suppress the UserWarning
    ani = FuncAnimation(fig, update, fargs=(ser, lines, axes_flat, temp),
                        frames=None, interval=GRAPH_UPDATE_INTERVAL,
                        blit=False, cache_frame_data=False) # Set blit=True for performance if init_func is also used correctly.

    plt.tight_layout(pad=2.0)
    plt.show()

    # It's good practice to close the serial port when done
    # This will run after the plot window is closed
    if ser.is_open:
        ser.close()
        print("Serial port closed.")