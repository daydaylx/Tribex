#!/usr/bin/env python3
"""
Generate test 909-style samples for TribeX development
Creates simple sine/noise-based drum samples
"""

import numpy as np
import struct
import os

SAMPLE_RATE = 44100
BIT_DEPTH = 16

def write_wav(filename, audio_data, sample_rate=44100):
    """Write audio data to WAV file"""
    audio_data = np.clip(audio_data, -1.0, 1.0)
    audio_data = (audio_data * 32767).astype(np.int16)
    
    with open(filename, 'wb') as f:
        # RIFF header
        f.write(b'RIFF')
        file_size = 36 + len(audio_data) * 2
        f.write(struct.pack('<I', file_size))
        f.write(b'WAVE')
        
        # fmt chunk
        f.write(b'fmt ')
        f.write(struct.pack('<I', 16))  # chunk size
        f.write(struct.pack('<H', 1))   # PCM format
        f.write(struct.pack('<H', 1))   # mono
        f.write(struct.pack('<I', sample_rate))
        f.write(struct.pack('<I', sample_rate * 2))  # byte rate
        f.write(struct.pack('<H', 2))   # block align
        f.write(struct.pack('<H', 16))  # bits per sample
        
        # data chunk
        f.write(b'data')
        f.write(struct.pack('<I', len(audio_data) * 2))
        audio_data.tofile(f)

def generate_kick():
    """Generate 909-style kick drum"""
    duration = 0.5
    samples = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, samples)
    
    # Pitched sine with envelope
    freq = 60 + 100 * np.exp(-t * 20)
    sine = np.sin(2 * np.pi * freq * t)
    
    # Envelope
    env = np.exp(-t * 8)
    
    # Click
    click = np.exp(-t * 200) * 0.3
    
    kick = (sine * env * 0.7 + click) * 0.9
    return kick

def generate_snare():
    """Generate 909-style snare"""
    duration = 0.3
    samples = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, samples)
    
    # Tone component
    freq = 200
    tone = np.sin(2 * np.pi * freq * t) * 0.3
    
    # Noise component
    noise = np.random.randn(samples) * 0.4
    
    # Envelope
    env = np.exp(-t * 15)
    
    snare = (tone + noise) * env * 0.8
    return snare

def generate_hihat(open=False):
    """Generate 909-style hihat"""
    duration = 0.3 if open else 0.08
    samples = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, samples)
    
    # High-frequency noise
    noise = np.random.randn(samples)
    
    # High-pass filter (simple)
    hihat = np.diff(noise, prepend=0) * 0.5
    
    # Envelope
    decay = 5 if open else 40
    env = np.exp(-t * decay)
    
    hihat = hihat * env * 0.6
    return hihat

def generate_clap():
    """Generate 909-style clap"""
    duration = 0.2
    samples = int(SAMPLE_RATE * duration)
    
    # Multiple short bursts
    clap = np.zeros(samples)
    for i in range(3):
        start = int(i * SAMPLE_RATE * 0.03)
        burst_len = int(SAMPLE_RATE * 0.01)
        if start + burst_len < samples:
            burst = np.random.randn(burst_len) * (1.0 - i * 0.2)
            clap[start:start + burst_len] += burst
    
    # Envelope
    t = np.linspace(0, duration, samples)
    env = np.exp(-t * 20)
    
    clap = clap * env * 0.7
    return clap

def generate_tom(freq=100):
    """Generate 909-style tom"""
    duration = 0.4
    samples = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, samples)
    
    # Pitched sine with envelope
    sine = np.sin(2 * np.pi * freq * t)
    
    # Envelope
    env = np.exp(-t * 10)
    
    tom = sine * env * 0.8
    return tom

def generate_crash():
    """Generate 909-style crash cymbal"""
    duration = 1.5
    samples = int(SAMPLE_RATE * duration)
    t = np.linspace(0, duration, samples)
    
    # Complex noise with multiple bands
    crash = np.random.randn(samples) * 0.4
    
    # Add some pitched components
    for freq in [3000, 4200, 5800, 7600]:
        crash += np.sin(2 * np.pi * freq * t) * 0.15 * np.random.random()
    
    # Envelope
    env = np.exp(-t * 2)
    
    crash = crash * env * 0.6
    return crash

def main():
    print("🎵 Generating TR-909 style test samples...\n")
    
    samples = {
        "909_kick.wav": generate_kick(),
        "909_snare.wav": generate_snare(),
        "909_clap.wav": generate_clap(),
        "909_clhat.wav": generate_hihat(open=False),
        "909_ohhat.wav": generate_hihat(open=True),
        "909_lotom.wav": generate_tom(freq=80),
        "909_hitom.wav": generate_tom(freq=180),
        "909_crash.wav": generate_crash()
    }
    
    for filename, audio in samples.items():
        write_wav(filename, audio)
        size = os.path.getsize(filename)
        print(f"✅ {filename}: {size:,} bytes")
    
    print(f"\n🎉 Generated {len(samples)} samples!")
    print("\nThese are synthetic test samples for development.")
    print("For production, use real TR-909 samples from:")
    print("  - https://freesound.org (search: 'TR-909')")
    print("  - https://samples.kb6.de/downloads.php")
    print("  - https://www.wave-alchemy.co.uk/free-samples/")

if __name__ == "__main__":
    main()
