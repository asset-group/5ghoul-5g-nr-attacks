import numpy as np 

# ================================
# import the gist module
import gist

# ====================== Setting up ===================

# Commented out for now

# Get the sampling frequency
#fs = gist.getSamplingFrequency()

# Set the sampling frequency
#gist.setSamplingFrequency (48000)

# Set the audio frame size that the module expects
#gist.setAudioFrameSize (512)

# ====================== Extracting Features ===================

# This example shows how to get features from a single audio frame

# Create an arbitrary sin wave as an audio frame
audioFrame = np.sin (np.arange((512.)) * np.pi / 180. * 4 )

# Process the audio frame
gist.processFrame (audioFrame)

# Now we extract features and print them out...

# ====================== Core Time Domain Features ===================

print("")
print("--- CORE TIME DOMAIN FEATURES ---")
print("")
print("RMS:", gist.rms())
print("Peak Energy:", gist.peakEnergy())
print("Zero Crossing Rate:", gist.zeroCrossingRate())
print("")

# ====================== Core Frequency Domain Features ===================

print("--- CORE FREQUENCY DOMAIN FEATURES ---")
print("")
print("Spectral Centroid: ", gist.spectralCentroid())
print("Spectral Crest:", gist.spectralCrest())
print("Spectral Flatness:", gist.spectralFlatness())
print("Spectral Rolloff:", gist.spectralRolloff())
print("Spectral Kurtosis:", gist.spectralKurtosis())
print("")

# ========================= Onset Detection Functions =======================

print("--- ONSET DETECTION FUNCTIONS ---")
print("")
print("Energy Difference:", gist.energyDifference())
print("Spectral Difference:", gist.spectralDifference())
print("Spectral Difference (half-wave rectified):", gist.spectralDifferenceHWR())
print("Complex Spectral Difference:", gist.complexSpectralDifference())
print("High Frequency Content:", gist.highFrequencyContent())
print("")

# ========================= Pitch =======================

print("--- PITCH ---")
print("")
print("Pitch:", gist.pitch())
print("")

# ======================= Spectra ========================

print("--- SPECTRA ---")
print("")
magnitudeSpectrum = gist.magnitudeSpectrum()
print("Magnitude Spectrum has", magnitudeSpectrum.size, "samples")

melFrequencySpectrum = gist.melFrequencySpectrum()
print("Mel-Frequency Spectrum has", melFrequencySpectrum.size, "samples")

mfccs = gist.mfccs()
print("MFCCs has", mfccs.size, "samples")


