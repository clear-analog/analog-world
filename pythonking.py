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
BAUD_RATE = 115200
PACKET_SIZE = 37            # Total bytes: 2 (start) + 1 (len) + 32 (payload) + 2 (end)

# --- Markers ---
START_MARKER = 0xABCD
END_MARKER = 0xDCBA

# --- Buffer for plotting ---
BUFFER_SIZE = 1000  # Number of samples to keep for plotting

# Each channel will have its own deque for y-values, and a shared deque for timestamps
channel_buffers = [deque(maxlen=BUFFER_SIZE) for _ in range(8)]
timestamp_buffer = deque(maxlen=BUFFER_SIZE)

# Thread-safe lock for buffer access
buffer_lock = threading.Lock()

def parse_packet(packet):
    if len(packet) != PACKET_SIZE:
        return

    # Unpack header
    start = (packet[0] << 8) | packet[1]
    length = packet[2]

    if start != START_MARKER:
        return

    if length != PACKET_SIZE:
        return

    # Unpack timestamp
    timestamp = struct.unpack('>I', packet[3:7])[0]

    # ADS1299 data (27 bytes: 8 channels * 3 bytes + 3 status bytes)
    ads_data = packet[7:34]

    # Checksum
    checksum = packet[34]
    computed_checksum = sum(packet[2:34]) & 0xFF

    if checksum != computed_checksum:
        return

    # End marker
    end = (packet[35] << 8) | packet[36]
    if end != END_MARKER:
        return

    # Parse 8 channels, each 3 bytes (24 bits, signed)
    channel_values = []
    for ch in range(8):
        idx = ch * 3
        raw = ads_data[idx:idx+3]
        # Convert 3 bytes to signed 24-bit int
        val = int.from_bytes(raw, byteorder='big', signed=False)
        if val & 0x800000:
            val = val - 0x1000000  # Convert to signed
        channel_values.append(val)
        #print("Channel 1 value", channel_values[0])

    # Add to buffer
    with buffer_lock:
        timestamp_buffer.append(timestamp)
        for ch in range(8):
            channel_buffers[ch].append(channel_values[ch])

def serial_thread():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        buffer = bytearray()
        while True:
            data = ser.read()  # Read byte-by-byte
            if data:
                buffer += data

                # Look for start marker
                while len(buffer) >= 2:
                    potential_start = (buffer[0] << 8) | buffer[1]
                    if potential_start == START_MARKER:
                        if len(buffer) >= PACKET_SIZE:
                            packet = buffer[:PACKET_SIZE]
                            buffer = buffer[PACKET_SIZE:]  # Remove processed packet
                            parse_packet(packet)
                        else:
                            break  # Wait for full packet
                    else:
                        buffer.pop(0)  # Discard leading byte until we hit start marker

# --- Plotly Dash App ---
app = Dash(__name__)

# Create 8 graphs, one for each channel
app.layout = html.Div([
    html.H1("ADS1299 8-Channel Live Data"),
    html.Div([
        html.Div([
            dcc.Graph(id=f'channel-{i+1}-graph')
        ], style={'width': '45%', 'display': 'inline-block', 'vertical-align': 'top', 'margin': '10px'})
        for i in range(8)
    ]),
    dcc.Interval(id='interval-component', interval=100, n_intervals=0)  # Update every 100 ms
])

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
            yaxis=dict(title='Value'),
            margin=dict(l=40, r=20, t=40, b=40),
            height=300
        )
        return go.Figure(data=[trace], layout=layout)

    return update_channel_graph

# Register all callbacks
callbacks = [generate_callback(i) for i in range(8)]



def main():
    # Start serial reading thread
    t = threading.Thread(target=serial_thread, daemon=True)
    t.start()
    # Run Dash app
    app.run(debug=True, use_reloader=False)

if __name__ == "__main__":
    main()
