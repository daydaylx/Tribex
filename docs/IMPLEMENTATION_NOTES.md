# TribeX Implementation Notes

## M4.5: Trim UI, Waveform Preview, Solo Logic (08.01.2026)

### Summary
- Implemented Trim UI with waveform preview in Sample Screen
- Added Solo logic to Audio Engine (only soloed parts play)
- Eliminated variable-length arrays from audio callback (C++17 compliance)
- Updated MinSdk from 24 to 26 (AAudio direct support)
- Updated Compose and Kotlin versions
- Added C++ unit tests for LockFreeQueue and Probability
- Build: SUCCESSFUL

### C++ Audio Engine (M4.5)
- **Preallocated Buffers**: Replaced VLAs with std::array in AudioEngine
  - `MAX_FRAMES_PER_CALLBACK = 1024`
  - `mPartLeftBuffer`, `mPartRightBuffer` for drum parts
  - `mSynthLeftBuffer`, `mSynthRightBuffer` for synth part
  - Zero-initialized in constructor, reused in callback
- **Solo Logic**: Implemented in `onAudioReady()`
  - `mAnyPartSoloed` atomic tracks if any part is soloed
  - Only soloed parts render when any part is soloed
  - Muted parts never render
- **Solo Update**: `setPartSolo()` updates `mAnyPartSoloed` flag

### Kotlin UI (M4.5)
- **WaveformData**: New data class for downsampled waveform
  - `fromAudioData()`: Converts raw float32 ByteArray to downsampled points
  - `downsample()`: Peak-hold algorithm for visualization
  - Max 1000 points for efficient rendering
- **WaveformPreview**: Canvas-based waveform visualization
  - Symmetric vertical bar graph
  - Blue for active range, dimmed for trim range
  - Red trim lines for start/end positions
- **TrimSlider**: RangeSlider for trim control
  - Integrated into PartSampleCard
  - Updates SampleMetadata.startOffset/endOffset
- **SampleMetadata Extended**: Added `waveform` field
  - Loaded on sample import (IO thread)
  - Consumed by UI thread (no blocking)

### Build Configuration (M4.5)
- **MinSdk**: Updated from 24 to 26
  - Rationale: AAudio direct support (99%+ device coverage)
  - Eliminates OpenSL ES fallback complexity
- **Dependencies Updated**:
  - Compose BOM: 2023.10.01 ’ 2024.02.01
  - Compose Compiler: 1.4.8 ’ 1.5.4
  - Lifecycle: 2.6.2 ’ 2.7.0
  - Activity Compose: 1.8.1 ’ 1.8.2
- **Version**: 0.1.0 ’ 0.2.0

### Unit Tests (M4.5)
- **AudioEngineTest.cpp**: New test suite
  - LockFreeQueue: push/pop, multiple operations, overflow handling
  - Probability: deterministic triggers, seed variance, loop consistency
  - AudioEvent: validation
  - AudioEngine: initialization, buffer size (compile-time check)

### Realtime Audio Rules
-  Audio callback does NO allocations (preallocated std::array)
-  All parameter updates use atomics
-  Sample loading on IO thread (not audio thread)
-  Voice stealing on audio thread (no blocking)
-  Linear interpolation resampling on audio thread
-  Solo logic uses atomics (no locks)

### Technical Notes (M4.5)
- Waveform downsampling uses peak-hold for better visualization
- Solo logic is atomic (single `mAnyPartSoloed` flag)
- Trim is non-destructive (original sample data preserved)
- Trim updates via JNI (future implementation, currently UI-only)
- Fixed buffer size of 1024 frames covers all typical audio callbacks

### Known Issues / Deferred
- **Trim Persistence**: Trim values not persisted to storage (deferred to M6)
- **Trim JNI Bridge**: Trim updates not sent to AudioEngine (deferred to M4.6)
- **Waveform Cache**: Waveform data recalculated on reload (acceptable for M4.5)
- **Touch Handling**: TrimSlider is visual-only, full touch handling deferred to M5
- **Filter**: Basic 1-pole implementation only (sophisticated filters deferred to M5)
- **Stereo Samples**: Current implementation is mono-only (stereo support deferred)
- **Sample Preview**: No preview playback in file picker (deferred to M4.5)

### Build Verification
```bash
./gradlew assembleDebug
# BUILD SUCCESSFUL in 7s
```

---

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