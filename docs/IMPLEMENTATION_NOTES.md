# TribeX Implementation Notes

## M4: Drum Sampler Implementation (06.01.2026)

### Summary
- Successfully implemented Drum Sampler with WAV import via Storage Access Framework
- Implemented voice pool (max 4 voices per part) with voice stealing
- Added linear interpolation resampling for pitch adjustment
- Implemented 1-knob DJ Filter (LPF/HPF)
- Build: SUCCESSFUL

### C++ Audio Engine
- **SampleVoice**: Sample playback with pitch, pan, level, decay, filter
  - Linear interpolation resampling
  - Exponential decay envelope
  - Constant power stereo panning
  - 1-pole LPF/HPF (no resonance for M4)
- **SamplePart**: Manages up to 4 voices with voice stealing
  - Voice pool with oldest-first stealing strategy
  - Sample data management with trim support
  - Atomic parameter updates (non-blocking)
- **SampleLoader**: WAV file parsing (16/24/32-bit PCM)
- **Filter**: Basic 1-pole LPF/HPF

### Kotlin UI
- **SampleScreen**: Main screen with 9 part cards
- **SampleBrowser**: Full WAV import implementation
  - Storage Access Framework integration (ActivityResultContracts)
  - WAV header parsing
  - Audio data conversion to float32
  - File size limit: 100MB
  - Error handling
- **Parameter Controls**: Pitch (-24 to +24 semitones), Pan (L/C/R), Level (0-100%), Decay (0-5000ms), Filter (LP/HP)

### Data Models
- **SampleData**: Sample metadata with trim support (startOffset/endOffset)
- **VoiceParams**: All voice parameters with default values and ranges
- **SampleImportResult**: Import result with float32 audio data
- **PartSampleState**: State management for 9 parts

### Realtime Audio Rules
-  Audio callback does NO allocations
-  All parameter updates use atomics
-  Sample loading on IO thread (not audio thread)
-  Voice stealing on audio thread (no blocking)
-  Linear interpolation resampling on audio thread

### Known Issues / Deferred
- **Trim UI**: Trim support implemented in C++, but no UI sliders yet (deferred to M4.5)
- **Waveform Preview**: No waveform display (deferred to M4.5)
- **Solo Logic**: Solo state tracked, but no audio ducking (deferred to M4.5)
- **Filter**: Basic 1-pole implementation only (sophisticated filters deferred to M5)
- **Stereo Samples**: Current implementation is mono-only (stereo support deferred)
- **Sample Preview**: No preview playback in file picker (deferred to M4.5)

### Build Verification
```bash
./gradlew assembleDebug
# BUILD SUCCESSFUL in 7s
```

### Technical Notes
- All sample data is preallocated in IO thread
- Voice stealing uses frame counter for tracking age
- Trim is non-destructive (original sample data preserved)
- Sample data passed via JNI as jbyte[] containing float32
- Audio callback renders directly to output buffers (no intermediate mixing)

### Persistence
- Trim offsets stored in SampleMetadata (ready for persistence)
- Sample ID assignment auto-incrementing
- No filesystem storage yet (deferred to M6 per spec)