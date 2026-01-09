# TribeX - 909 Drum Samples

## 📁 Samples Overview

| Part | File | Description | Format |
|------|------|-------------|---------|
| 0 | `909_kick.wav` | Bass Drum (BD) | 16-bit 44.1kHz Mono |
| 1 | `909_snare.wav` | Snare Drum (SD) | 16-bit 44.1kHz Mono |
| 2 | `909_clap.wav` | Hand Clap (CP) | 16-bit 44.1kHz Mono |
| 3 | `909_clhat.wav` | Closed Hi-Hat (CH) | 16-bit 44.1kHz Mono |
| 4 | `909_ohhat.wav` | Open Hi-Hat (OH) | 16-bit 44.1kHz Mono |
| 5 | `909_lotom.wav` | Low Tom (LT) | 16-bit 44.1kHz Mono |
| 6 | `909_hitom.wav` | High Tom (HT) | 16-bit 44.1kHz Mono |
| 7 | `909_crash.wav` | Crash Cymbal (CY) | 16-bit 44.1kHz Mono |

---

## ✨ Sample Details

### Current Samples
- **Type**: Synthetically generated TR-909 style test samples
- **Purpose**: Development and testing
- **Quality**: Basic (for prototyping)

These samples were programmatically generated using:
- Sine waves for tonal elements (kick, toms)
- White noise for percussive elements (snare, hihat)
- Exponential envelopes for natural decay

### Recommended Production Samples

For better sound quality, replace with authentic samples from:

1. **Free Sources:**
   - [Freesound.org](https://freesound.org) - Search: "TR-909" (CC licenses)
   - [Wave Alchemy](https://www.wave-alchemy.co.uk/free-samples/) - Free packs
   - [Samples.kb6.de](https://samples.kb6.de/downloads.php) - Drum machine samples

2. **Commercial (High Quality):**
   - [Goldbaby](https://www.goldbaby.co.nz) - Legendary 909 samples
   - [Wave Alchemy](https://www.wave-alchemy.co.uk) - Professional packs
   - [Samples From Mars](https://samplesfrommars.com) - Authentic hardware recordings

---

## 🔧 Using Custom Samples

### Replace Individual Samples

1. Navigate to `app/src/main/assets/samples/`
2. Replace any `909_*.wav` file with your own
3. **Requirements:**
   - Format: WAV (PCM)
   - Bit depth: 16-bit or 24-bit
   - Channels: Mono (stereo will be converted)
   - Sample rate: Any (44.1kHz or 48kHz recommended)

### Adding New Kits

Create subdirectories for different kits:
```
app/src/main/assets/samples/
├── 909_kick.wav         # Default kit (root)
├── 909_snare.wav
├── ...
├── 808/                 # TR-808 kit
│   ├── 808_kick.wav
│   └── ...
└── acoustic/            # Acoustic kit
    ├── acoustic_kick.wav
    └── ...
```

---

## 📝 Naming Convention

Samples must follow this pattern:
```
{kit}_{instrument}.wav
```

**Instruments:**
- `kick` - Bass drum (Part 0)
- `snare` - Snare drum (Part 1)
- `clap` - Hand clap (Part 2)
- `clhat` - Closed hi-hat (Part 3)
- `ohhat` - Open hi-hat (Part 4)
- `lotom` - Low tom (Part 5)
- `hitom` - High tom (Part 6)
- `crash` - Crash cymbal (Part 7)

---

## 🚀 Integration Status

- ✅ Samples copied to `app/src/main/assets/samples/`
- ✅ Format: 16-bit WAV, Mono, 44.1kHz
- ⏳ Sample loader implementation (TODO)
- ⏳ Kit switcher UI (TODO)

---

## 🎹 Sample Characteristics (Current)

| Sample | Duration | Envelope | Frequency Content |
|--------|----------|----------|-------------------|
| Kick | 500ms | 8 Hz decay | 60-160 Hz (swept) |
| Snare | 300ms | 15 Hz decay | 200 Hz + noise |
| Clap | 200ms | 20 Hz decay | 3x burst noise |
| CH | 80ms | 40 Hz decay | High-pass noise |
| OH | 300ms | 5 Hz decay | High-pass noise |
| LT | 400ms | 10 Hz decay | 80 Hz sine |
| HT | 400ms | 10 Hz decay | 180 Hz sine |
| Crash | 1500ms | 2 Hz decay | Multi-band noise |

---

## 📊 File Sizes

```
909_kick.wav   :  44 KB
909_snare.wav  :  27 KB
909_clap.wav   :  18 KB
909_clhat.wav  :   7 KB
909_ohhat.wav  :  27 KB
909_lotom.wav  :  35 KB
909_hitom.wav  :  35 KB
909_crash.wav  : 132 KB
----------------------------
TOTAL          : 325 KB
```

Sehr kompakt für das APK (< 400 KB für komplettes Drum-Kit)!

---

## 🎵 Next Steps

1. **Sample Loader implementieren**
   - Kotlin: AssetManager zum Laden der WAVs
   - JNI-Bridge: Samples an C++ Audio-Engine übergeben
   
2. **Sample-Browser UI**
   - Liste aller verfügbaren Samples
   - Preview-Funktion
   - Assign to Part

3. **Multi-Kit-Support**
   - Kit-Auswahl-Menü
   - Presets speichern/laden

---

**Generated**: 9. Januar 2026  
**Status**: Development Samples Ready ✅
