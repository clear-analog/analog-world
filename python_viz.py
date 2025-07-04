import serial
import struct
import threading
import time
from collections import deque

import plotly.graph_objs as go
from plotly.subplots import make_subplots
from dash import Dash, dcc, html, Output, Input, State
import dash
import dash.dependencies

# --- Serial Configuration ---
SERIAL_PORT = 'COM4'        # Change this to your port, e.g. /dev/ttyUSB0 on Linux/Mac
BAUD_RATE = 921600

# --- Packet Structure (from firmware) ---
# 2 bytes: start marker (0xABCD, big endian)
# 1 byte:  length (should be 31: 4 timestamp + 27 data)
# 4 bytes: timestamp (big endian)
# 27 bytes: ADS1299 data (3 status + 8*3 channel bytes)
# 1 byte: checksum (sum of length through last data byte, modulo 256)
# 2 bytes: end marker (0xDCBA, big endian)
PACKET_START_MARKER = 0xABCD
PACKET_END_MARKER = 0xDCBA
PACKET_START_MARKER_BYTES = 2
PACKET_LENGTH_BYTES = 1
PACKET_TIMESTAMP_BYTES = 4
ADS1299_NUM_STATUS_BYTES = 3
ADS1299_NUM_CHANNELS = 8
ADS1299_BYTES_PER_CHANNEL = 3
ADS1299_TOTAL_DATA_BYTES = ADS1299_NUM_STATUS_BYTES + (ADS1299_NUM_CHANNELS * ADS1299_BYTES_PER_CHANNEL)
PACKET_MSG_LENGTH = PACKET_TIMESTAMP_BYTES + ADS1299_TOTAL_DATA_BYTES  # 4 + 27 = 31
PACKET_CHECKSUM_BYTES = 1
PACKET_END_MARKER_BYTES = 2
PACKET_TOTAL_SIZE = (
    PACKET_START_MARKER_BYTES +
    PACKET_LENGTH_BYTES +
    PACKET_MSG_LENGTH +
    PACKET_CHECKSUM_BYTES +
    PACKET_END_MARKER_BYTES
)  # 2 + 1 + 31 + 1 + 2 = 37

# --- Buffer for plotting ---
BUFFER_SIZE = 1000  # Number of samples to keep for plotting

# Each channel will have its own deque for y-values, and a shared deque for timestamps
channel_buffers = [deque(maxlen=BUFFER_SIZE) for _ in range(ADS1299_NUM_CHANNELS)]
timestamp_buffer = deque(maxlen=BUFFER_SIZE)

# For sample rate calculation: store the last N timestamps per channel
SPS_WINDOW = 100  # Number of samples to use for SPS calculation
channel_timestamp_buffers = [deque(maxlen=SPS_WINDOW) for _ in range(ADS1299_NUM_CHANNELS)]

# Thread-safe lock for buffer access
buffer_lock = threading.Lock()

# --- Parse Packet ---
def parse_packet(packet):
    if len(packet) != PACKET_TOTAL_SIZE:
        return

    # Unpack start marker (big endian)
    start = (packet[0] << 8) | packet[1]
    if start != PACKET_START_MARKER:
        return

    # Length field (should be 31)
    length = packet[2]
    if length != PACKET_MSG_LENGTH:
        return

    # Timestamp (4 bytes, big endian)
    timestamp = struct.unpack('>I', packet[3:7])[0]

    # ADS1299 data (27 bytes: 3 status + 8*3 channel bytes)
    ads_data = packet[7:7+ADS1299_TOTAL_DATA_BYTES]
    if len(ads_data) != ADS1299_TOTAL_DATA_BYTES:
        return

    # Checksum (sum of all bytes from length to last data byte, modulo 256)
    checksum = packet[7+ADS1299_TOTAL_DATA_BYTES]
    computed_checksum = sum(packet[2:7+ADS1299_TOTAL_DATA_BYTES]) & 0xFF
    if checksum != computed_checksum:
        return

    # End marker (big endian)
    end = (packet[-2] << 8) | packet[-1]
    if end != PACKET_END_MARKER:
        return

    # Parse 8 channels, each 3 bytes (24 bits, signed)
    channel_values = []
    for ch in range(ADS1299_NUM_CHANNELS):
        idx = ADS1299_NUM_STATUS_BYTES + ch * ADS1299_BYTES_PER_CHANNEL
        raw = ads_data[idx:idx+ADS1299_BYTES_PER_CHANNEL]
        # Convert 3 bytes to signed 24-bit int (big endian)
        value = int.from_bytes(raw, byteorder='big', signed=False)
        #value = raw[0] << 16 | raw[1] << 8 | raw[2]

        # If that MSB was actually 1 which implies that it was a negative number, so signed extension is now necessary
        #if (value & 0b00000000100000000000000000000000):
        #    value = value | 0b11111111000000000000000000000000

        if value & 0b00000000100000000000000000000000:
            value = value - 0x1000000  # Convert to signed
        ##    val = val | 0b11111111000000000000000000000000
        v = convert_to_volt(value)
        channel_values.append(v)

    # Add to buffer
    with buffer_lock:
        timestamp_buffer.append(timestamp)
        for ch in range(ADS1299_NUM_CHANNELS):
            channel_buffers[ch].append(channel_values[ch])
            channel_timestamp_buffers[ch].append(timestamp)

# --- Serial Messaging Thread ---
def serial_thread():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        buffer = bytearray()
        while True:
            data = ser.read()  # Read byte-by-byte
            if data:
                buffer += data

                # Look for start marker
                while len(buffer) >= PACKET_TOTAL_SIZE:
                    # Check for start marker at buffer[0:2]
                    start = (buffer[0] << 8) | buffer[1]
                    if start == PACKET_START_MARKER:
                        # Check if we have a full packet
                        if len(buffer) >= PACKET_TOTAL_SIZE:
                            packet = buffer[:PACKET_TOTAL_SIZE]
                            # Validate end marker before parsing
                            end = (packet[-2] << 8) | packet[-1]
                            if end == PACKET_END_MARKER:
                                parse_packet(packet)
                                buffer = buffer[PACKET_TOTAL_SIZE:]
                            else:
                                # Bad end marker, discard first byte and resync
                                buffer = buffer[1:]
                        else:
                            break  # Wait for full packet
                    else:
                        # Not a start marker, discard first byte and resync
                        buffer = buffer[1:]

# This function converts the ADC count into a voltage value
def convert_to_volt(raw_val, vref=4.5, gain=24):
    full_scale = (2 * vref / gain) / (2 ** 24) # Full-scale range in Volts
    return(raw_val * full_scale) # outputting in volt

# --- Plotly Dash App ---
app = Dash(__name__)

# Create 8 graphs, one for each channel, and a histogram for SPS
app.layout = html.Div([
    html.H1("ADS1299 8-Channel Live Data"),
    html.Div([
        html.Div([
            dcc.Graph(id=f'channel-{i+1}-graph')
        ], style={'width': '45%', 'display': 'inline-block', 'vertical-align': 'top', 'margin': '10px'})
        for i in range(ADS1299_NUM_CHANNELS)
    ]),
    html.Div([
        dcc.Graph(id='sps-histogram')
    ], style={'width': '95%', 'display': 'block', 'margin': '20px auto'}),
    dcc.Interval(id='interval-component', interval=100, n_intervals=0),  # Update every 100 ms for channels and SPS
])

# To be called in a loop for each channel
def generate_callback(ch_idx):
    @app.callback(
        Output(f'channel-{ch_idx+1}-graph', 'figure'),
        [Input('interval-component', 'n_intervals')],
        [State(f'channel-{ch_idx+1}-graph', 'figure')]
    )
    def update_channel_graph(n, fig):
        with buffer_lock:
            y = list(channel_buffers[ch_idx])
            x = list(timestamp_buffer)
        if not x:
            x = [0]
            y = [0]
        trace = go.Scatter(x=x, y=y, mode='lines', name=f'Channel {ch_idx+1}')
        layout = go.Layout(
            title=f'Channel {ch_idx+1}',
            xaxis=dict(title='Timestamp (ms)'),
            yaxis=dict(title='Voltage (V)'),  # Changed from uV to V
            margin=dict(l=40, r=20, t=40, b=40),
            height=300
        )
        return go.Figure(data=[trace], layout=layout)

    return update_channel_graph

# Callback for SPS histogram
@app.callback(
    Output('sps-histogram', 'figure'),
    [Input('interval-component', 'n_intervals')]
)
def update_sps_histogram(n):
    sps_values = []
    with buffer_lock:
        for ch in range(ADS1299_NUM_CHANNELS):
            ts_buf = channel_timestamp_buffers[ch]
            if len(ts_buf) > 50:
                idx1 = 0
                idx2 = 50
                t1 = ts_buf[idx1]
                t2 = ts_buf[idx2]
                dt = (t2 - t1) / 1000.0  # ms to s
                num_samples = idx2 - idx1
                sps = num_samples / dt if dt > 0 else 0
            else:
                sps = 0
            sps_values.append(sps)
    bar = go.Bar(
        x=[f'Ch {i+1}' for i in range(ADS1299_NUM_CHANNELS)],
        y=sps_values,
        marker=dict(color='royalblue')
    )
    layout = go.Layout(
        title='Samples Per Second (SPS) per Channel',
        xaxis=dict(title='Channel'),
        yaxis=dict(title='SPS', range=[0, max(sps_values + [1]) * 1.2]),
        bargap=0.3,
        height=350,
        margin=dict(l=40, r=20, t=40, b=40)
    )
    return go.Figure(data=[bar], layout=layout)
# Register all callbacks
callbacks = [generate_callback(i) for i in range(ADS1299_NUM_CHANNELS)]

def main():
    # Start serial reading thread
    t = threading.Thread(target=serial_thread, daemon=True)
    t.start()
    # Run Dash app
    app.run(debug=True, use_reloader=False)

if __name__ == "__main__":
    main()