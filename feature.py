import numpy as np
import pandas as pd
from scipy.fft import fft, fftfreq

#we compute fft here
def compute_fft(signal, sampling_rate):

    N = len(signal)
    T = 1 / sampling_rate

    fft_values = fft(signal)
    freqs = fftfreq(N, T)

    magnitude = np.abs(fft_values)

    half = N // 2
    # normalization (safe guard)
    if np.max(magnitude) != 0:
        magnitude_norm = magnitude / np.max(magnitude)
    else:
        magnitude_norm = magnitude

    return freqs[:half], magnitude[:half], magnitude_norm[:half]
  






#feature extraction function

def extract_features(signal, freqs, mag):
    

    N = len(mag)

    

    # ----------------------------
    # Frequency band splitting
    # ----------------------------
    low_band = mag[0:int(0.1 * N)]
    mid_band = mag[int(0.1 * N):int(0.5 * N)]
    high_band = mag[int(0.5 * N):]

    # ----------------------------
    # Core features
    # ----------------------------

    # Energy in each band
    low_energy = np.sum(low_band)
    mid_energy = np.sum(mid_band)
    high_energy = np.sum(high_band)

    # Peak frequency index
    peak_index = np.argmax(mag)
    peak_freq = freqs[peak_index] if len(freqs) == len(mag) else peak_index

    # RMS (overall vibration strength)
    rms = np.sqrt(np.mean(signal ** 2)) if signal is not None else np.sqrt(np.mean(mag ** 2))

    # Spectral centroid (important for sound analysis)
    freq_bins = np.arange(len(mag))
    spectral_centroid = np.sum(freq_bins * mag) / (np.sum(mag) + 1e-8)



        # 1. RMS
    rms = np.sqrt(
        np.mean(signal**2)
    )


    # 2. Variance
    variance = np.var(signal)



    # 3. Zero Crossing Rate
    zero_crossings = np.where(
        np.diff(np.sign(signal))
    )[0]

    zero_crossing_rate = (
        len(zero_crossings)
        /
        len(signal)
    )

     # 4. Peak count
    threshold = np.mean(
        np.abs(signal)
    ) + np.std(signal)


    peaks = np.where(
        np.abs(signal) > threshold
    )[0]


    peak_count = len(peaks)



    # 5. Kurtosis
    mean = np.mean(signal)

    std = np.std(signal)

    kurtosis = (
        np.mean((signal-mean)**4)
        /
        (std**4 + 1e-8)
    )


    # ----------------------------
    # Final feature vector
    # ----------------------------
    features = [
        low_energy,
        mid_energy,
        high_energy,
        peak_freq,  
        spectral_centroid,
        rms,
        variance,
        zero_crossing_rate,
        peak_count,
        kurtosis
        
    ]

    return features
