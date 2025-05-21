import struct
import serial
import numpy as np
import time
from collections import deque # Still a good idea for later
from matplotlib.animation import FuncAnimation
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors # For distinct line colors

# --- Matplotlib Backend (Optional, but can help if FuncAnimation isn't firing) ---
# import matplotlib
# matplotlib.use('TkAgg') # Or 'Qt5Agg', 'GTK3Agg'
# import matplotlib.pyplot as plt
# ---------------------------------------------------------------------------------


# Necessary Global Variables (consider reducing global scope later)
SERIAL_PORT = "COM4" # Make sure this is the correct COM port
SERIAL_PORT_BAUD = 115200
EEG_CHANNEL_NUM = 8
EEG_CHANNEL_BYTES = 3
STATUS_BYTES = 3
TIMESTAMP_BYTES = 4
SERIAL_MSG_SIZE = TIMESTAMP_BYTES + STATUS_BYTES + EEG_CHANNEL_NUM * EEG_CHANNEL_BYTES
GRAPH_TIME_WINDOW = 10  # seconds (Increased from 4 to 10 seconds for a larger x-range)
GRAPH_UPDATE_INTERVAL = 100  # ms (This is how often `update` is CALLED)

# Global data stores - consider using deques for automatic size management
T_DATA = deque(maxlen=5000) # Store approx 50 seconds of data if 100ms interval
EEG_DATA = [deque(maxlen=5000) for _ in range(EEG_CHANNEL_NUM)]

# Global Serial Object - initialized in main
SERIAL_OBJECT = None

def parse_signed_24bit_big_endian(three_bytes):
    """Correctly parses a 3-byte big-endian signed integer."""
    if not three_bytes or len(three_bytes) != 3:
        return 0 # Or raise an error, or return NaN
    # Prepend a sign-extending byte if the MSB of the 3 bytes is 1
    if three_bytes[0] & 0x80:
        val = struct.unpack('>i', b'\xff' + three_bytes)[0]
    else:
        val = struct.unpack('>i', b'\x00' + three_bytes)[0]
    return val

def update(frame, ser_obj, lines_list, ax_plot, temp_debug_list):
    #print(f"--- Update called: frame {frame}, time: {time.time():.2f}, serial waiting: {ser_obj.in_waiting if ser_obj and ser_obj.is_open else 'N/A'} ---")
    
    data_actually_read_this_frame = False
    if ser_obj and ser_obj.is_open and ser_obj.in_waiting >= SERIAL_MSG_SIZE:
        raw_packet = ser_obj.read(SERIAL_MSG_SIZE)
        data_actually_read_this_frame = True
        #print(f"Read {len(raw_packet)} bytes. Expected {SERIAL_MSG_SIZE}.")

        timestamp_ms = struct.unpack('<L', raw_packet[:4])[0]
        # status_bytes = raw_packet[4: 4+STATUS_BYTES] # Read but not used
        channel_data_bytes = raw_packet[4+STATUS_BYTES:]
        
        temp_debug_list.append(timestamp_ms) # 'temp_debug_list'
        T_DATA.append(timestamp_ms)

        current_voltages_str = []
        for i in range(EEG_CHANNEL_NUM):
            this_channel_bytes = channel_data_bytes[i*EEG_CHANNEL_BYTES : i*EEG_CHANNEL_BYTES+EEG_CHANNEL_BYTES]

            if len(this_channel_bytes) == EEG_CHANNEL_BYTES:
                value = parse_signed_24bit_big_endian(this_channel_bytes)
                # Voltage calculation (e.g., for ADS1299 with VREF=2.5V, Gain=1 (can be adjusted))
                # LSB size = (2 * VREF / Gain) / (2^24)
                # voltage = value * (2.5 / (2**23 -1)) * 1e6 # in uV if VREF is in V
                # Simplified version from your code for now:
                voltage = value * 2.5 * 1000000 / (2**23) # Check this formula carefully for your ADC
                
                temp_debug_list.append(voltage)
                EEG_DATA[i].append(voltage)
                current_voltages_str.append(f"Ch{i}:{voltage:.2f}uV")
            else:
                EEG_DATA[i].append(np.nan) # Append NaN if data is malformed for this channel
                temp_debug_list.append(np.nan)
                current_voltages_str.append(f"Ch{i}:NaN")
        
        print(f"TS: {timestamp_ms}, Voltages: [{', '.join(current_voltages_str)}]")

    elif ser_obj and ser_obj.is_open:
        # This means update was called, but not enough data was waiting.
        # This will print "SEXY TIMEs" if this condition is met.
        print(f'SEXY TIMEs (Not enough data: {ser_obj.in_waiting} bytes waiting, need {SERIAL_MSG_SIZE})')
    elif not (ser_obj and ser_obj.is_open):
        print("Serial object not available or not open in update function!")
        return lines_list # Nothing to do

    # Update the plot lines
    # This section will run regardless of whether new data was read this frame,
    # which is important for blitting or if axis limits change.
    # However, we only update data if new data actually came in or if T_DATA has something.

    if T_DATA: # Only update if there's some data history
        min_len = len(T_DATA)
        for i in range(EEG_CHANNEL_NUM):
            min_len = min(min_len, len(EEG_DATA[i]))
        
        # Use a consistent slice of T_DATA for all EEG channels
        # This ensures all lines get data up to the same point in time available across all channels
        # (assuming EEG_DATA appends NaNs if a specific channel's data is missing for a timestamp)
        # If using deques, direct conversion to list is fine:
        plot_t_data = list(T_DATA)[-min_len:]

        for i, line in enumerate(lines_list):
            plot_eeg_data = list(EEG_DATA[i])[-min_len:]
            line.set_data(plot_t_data, plot_eeg_data)

        # X-axis scrolling logic:
        if plot_t_data:
            latest_time = plot_t_data[-1]
            xmin = latest_time - (GRAPH_TIME_WINDOW * 1000)
            xmax = latest_time
            #ax_plot.set_xlim(xmin, xmax)
            # print(f"Plot X-limits: ({xmin:.0f}, {xmax:.0f})") # DEBUG

        # Autoscale Y-axis based on visible data
        ax_plot.relim()
        ax_plot.autoscale_view(scalex=True, scaley=True) # X is manually set, Y autoscales
        # print(f"Plot Y-limits (auto): {ax_plot.get_ylim()}") # DEBUG
    
    return lines_list

def setup_plot():
    global SERIAL_OBJECT
    try:
        SERIAL_OBJECT = serial.Serial(SERIAL_PORT, SERIAL_PORT_BAUD, timeout=0.05) # Small timeout
        print(f"Successfully connected to {SERIAL_PORT} at {SERIAL_PORT_BAUD} baud.")
    except serial.SerialException as e:
        print(f"ERROR: Could not open serial port {SERIAL_PORT}: {e}")
        return None, None, None, None # Indicate failure

    fig, ax = plt.subplots(1, 1, figsize=(15, 7)) # Single subplot

    lines = []
    # Generate distinct colors for 8 lines
    colors = plt.cm.get_cmap('tab10', EEG_CHANNEL_NUM) # 'tab10' has 10 distinct colors

    for i in range(EEG_CHANNEL_NUM):
        line, = ax.plot([], [], lw=1.5, color=colors(i), label=f'EEG {i+1}')
        lines.append(line)
    
    ax.set_title('EEG Signals')
    ax.set_xlabel('Timestamp (ms)')
    ax.set_ylabel('Voltage (uV)')
    ax.legend(loc='upper left', fontsize='small') # Add legend
    ax.grid(True)

    initial_xlim_max = GRAPH_TIME_WINDOW * 1000
    #ax.set_xlim(0, initial_xlim_max)
    ax.set_ylim(-150, 150) # Initial Y limits, will autoscale

    return SERIAL_OBJECT, lines, ax, fig


if __name__ == "__main__":
    # `ser` from setup_plot will be the global SERIAL_OBJECT if successful
    ser, lines_list, ax_plot, fig = setup_plot()

    if ser is None: # If setup failed (e.g., serial port error)
        print("Exiting due to setup failure.")
        exit()

    temp_debug_data_collector = [] # For your original 'temp' list's purpose

    ani = FuncAnimation(fig, update, 
                        fargs=(ser, lines_list, ax_plot, temp_debug_data_collector),
                        frames=None, interval=GRAPH_UPDATE_INTERVAL,
                        blit=False, cache_frame_data=False)

    plt.tight_layout(pad=2.0)
    plt.show()

    # This runs after the plot window is closed
    if ser.is_open:
        ser.close()
        print("Serial port closed.")
    print("Program finished.")