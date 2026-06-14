import serial
import numpy as np
import pandas as pd
from collections import deque
import time
import os  # Added to safely handle file checking
from feature import extract_features, compute_fft


# SERIAL CONNECTION

try:
    ser = serial.Serial('COM8', 115200, timeout=1)
    time.sleep(2)
    print("Sensor connected successfully on COM8")
except Exception as e:
    print(f"Error connecting to Serial Port: {e}")
    exit()


# SETTINGS

window_size = 256
sampling_rate = 10  # 10 samples per second

seismic_buffer = deque(maxlen=window_size)
sound_buffer = deque(maxlen=window_size)

dataset = []


# MAIN DATA COLLECTION

print("\nEnter class name (walking, digging, vehicle, noise):")
label = input("Class: ")

samples_needed = 500
print(f"Collecting: {label}. Please wait, filling initial 25.6s buffer...")

count = 0
raw_line_counter = 0

while count < samples_needed:
    line = ser.readline().decode().strip()
    
    raw_line_counter += 1
    if raw_line_counter % 10 == 0 and len(seismic_buffer) < window_size:
        print(f"Receiving data... Buffer status: {len(seismic_buffer)}/{window_size}")

    values = line.split(',')

    if len(values) == 3:
        try:
            node = int(values[0])
            seismic = int(values[1])
            sound = int(values[2])
            
            seismic_buffer.append(seismic)
            sound_buffer.append(sound)
        except ValueError:
            continue

        # ONLY calculate features once the buffer is completely full
        if len(seismic_buffer) == window_size and len(sound_buffer) == window_size:
            
            seismic_signal = np.array(seismic_buffer)
            sound_signal = np.array(sound_buffer)

            sf, sm, sn = compute_fft(seismic_signal, sampling_rate)
            af, am, an = compute_fft(sound_signal, sampling_rate)

            seismic_features = extract_features(seismic_signal, sf, sm)
            sound_features = extract_features(sound_signal, af, am)

            row = seismic_features + sound_features + [label]
            dataset.append(row)

            count += 1
            print(f"Sample Collected: {count} / {samples_needed}")


# SAVE CSV (STRICT APPEND MODE)

columns = [
    "seismic_low_energy", "seismic_mid_energy", "seismic_high_energy", "seismic_peak_freq",
    "seismic_rms", "seismic_centroid", "seismic_variance", "seismic_zcr", "seismic_peak_count", "seismic_kurtosis",
    "sound_low_energy", "sound_mid_energy", "sound_high_energy", "sound_peak_freq",
    "sound_rms", "sound_centroid", "sound_variance", "sound_zcr", "sound_peak_count", "sound_kurtosis",
    "label"
]

# Convert newly collected samples to a dataframe
df_new = pd.DataFrame(dataset, columns=columns)

# Check if the file already exists on your laptop
file_exists = os.path.isfile("dataset.csv")

# Save using 'a' (append mode) instead of overwriting!
# If the file exists, don't write the column headers again (header=False)
df_new.to_csv("dataset.csv", mode='a', index=False, header=not file_exists)

print(f"\nSUCCESS - Added {count} new '{label}' rows to dataset.csv without overwriting!")