# analog-world

            """ # Only process and plot if we have enough samples for filtering (at least 2 seconds)
            # and if enough time has passed since the last plot update (for performance)
            if len(time_buffer) >= SAMPLE_RATE * 2 and current_time - last_plot_time > 0.1:
                last_plot_time = current_time
                
                # Create BrainFlow-compatible array: shape (channels, samples)
                data_array = np.array([list(buf) for buf in raw_data_buffers])
                
                # Apply filters for each frequency band
                filtered_data = apply_band_filters(data_array, SAMPLE_RATE)
                
                # Store filtered data in band-specific buffers
                for band_name, band_data in filtered_data.items():
                    for ch in range(NUM_CHANNELS):
                        # Append the most recent filtered sample to each band's channel buffer
                        filtered_bands[band_name][ch].append(band_data[ch, -1])
                
                # Prepare data for CSV
                csv_row = [current_time] + [data_array[ch, -1] for ch in range(NUM_CHANNELS)]
                for band in FREQ_BANDS:
                    for ch in range(NUM_CHANNELS):
                        csv_row.append(filtered_bands[band][ch][-1])
                
                # Write to CSV
                csv_writer.writerow(csv_row)

                # Get time window for plotting
                time_vals = np.array(list(time_buffer))
                current_time_window = time_vals[-1]
                start_time_window = max(0, current_time_window - DISPLAY_TIME_WINDOW)
                
                # Update plot ranges
                for ax in [raw_ax] + band_axes:
                    ax.set_xlim(start_time_window, current_time_window)
                
                # Update raw data plot
                for ch in range(NUM_CHANNELS):
                    raw_data = np.array(list(raw_data_buffers[ch]))
                    raw_lines[ch].set_data(time_vals, raw_data)
                
                # Auto-scale y-axis for raw plot
                raw_ax.relim()
                raw_ax.autoscale_view(scalex=False, scaley=True)
                
                # Update filtered band plots
                for band_idx, (band_name, band_ax, lines) in enumerate(zip(FREQ_BANDS.keys(), band_axes, band_lines)):
                    for ch in range(NUM_CHANNELS):
                        band_data = np.array(list(filtered_bands[band_name][ch]))
                        lines[ch].set_data(time_vals, band_data)
                    
                    # Auto-scale y-axis for each band plot
                    band_ax.relim()
                    band_ax.autoscale_view(scalex=False, scaley=True) """


# Add esptool within cerelog project folder to PATH. Even though esptool.py is where it needs to be, esp32.py can't find it (https://github.com/zephyrproject-rtos/zephyr/discussions/88424)