import serial
import matplotlib.pyplot as plt
from collections import deque
import numpy as np
import pandas as pd
import joblib

from feature import extract_features,compute_fft

model = joblib.load(
    "sound_seismic_model.pkl"
)

scaler = joblib.load(
    "scaler.pkl"
)

print("AI model loaded")

# serial connection

ser = serial.Serial('COM3', 115200)

# windowing

window_size = 256

# Stores latest values only(buffers)
seismic_buffer = deque(maxlen=window_size)
sound_buffer = deque(maxlen=window_size)

#how frequently reads the data?





#sampling rate need to update
sampling_rate = 10

data=[]
while True:
    line = ser.readline().decode().strip()

    values = line.split(',')
  

    if len(values)==3:

        seismic = int(values[1])
        sound = int(values[2])
        seismic_buffer.append(seismic)
        sound_buffer.append(sound)
        # -------------------------
        # WINDOW READY?
        # -------------------------

        if len(seismic_buffer) == window_size and len(sound_buffer) == window_size:

            # Convert to numpy array
            seismic_window = np.array(seismic_buffer)
            sound_window=np.array(sound_buffer)

            # FFT
            seismic_freq, seismic_mag, seismic_norm= compute_fft(seismic_window, sampling_rate)
            sound_freq, sound_mag, sound_norm = compute_fft(sound_window, sampling_rate)
            
            seismic_features = extract_features(seismic_window, seismic_freq, seismic_norm)
            sound_features   = extract_features(sound_window, sound_freq, sound_norm)
            features=seismic_features+sound_features

           


    


            # 5. Scale features

            features = scaler.transform(
            [features]
            )

             # 6. Predict

            result = model.predict(
            features
            )


            prob = model.predict_proba(features)

            confidence = np.max(prob)*100

            print(
            "Detected:",
            result[0],
            "Confidence:",
            round(confidence,2),
            "%"
            )