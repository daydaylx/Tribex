# TribeX Deferred Features

## M4 Deferred Items (completed but with limitations)

### Trim UI
- **Status**: M4.5
- **Reason**: Trim support implemented in C++ (startOffset/endOffset), but no UI sliders yet
- **Current State**: 
  - Trim parameters in SampleMetadata (startOffset, endOffset) ✅
  - Trim applied during sample loading in SamplePart::loadSample() ✅
  - Helper methods in SampleMetadata (getTrimRange, setTrimRange) ✅
  - No UI controls for trim adjustment
- **Deferred**: 
  - Trim slider UI (start/end offset in percentage or ms)
  - Visual feedback of trim region
  - Trim reset button

### Waveform Preview
- **Status**: M4.5
- **Reason**: Complex rendering requires careful optimization
- **Current State**: No waveform display
- **Deferred**:
  - Waveform extraction from sample data
  - Canvas rendering with zoom/pan
  - Performance optimization for large samples

### Solo Logic
- **Status**: M4.5
- **Reason**: Mute implemented (UI-only), but solo requires per-part volume ducking
- **Current State**: Solo state tracked but no audio effect
- **Deferred**: Implement solo audio logic (mute all non-soloed parts)

### Sample Preview
- **Status**: M4.5
- **Reason**: Requires additional audio engine features
- **Current State**: No preview playback in file picker
- **Deferred**:
  - Preview playback button
  - Preview voice (separate from pattern voices)
  - Auto-stop preview on new selection

## M5 Deferred Items

### Advanced Filters
- **Status**: M5
- **Reason**: M4 only requires basic LPF/HPF
- **Current State**: 1-pole LPF/HPF (no resonance, fixed cutoff)
- **Deferred**:
  - Biquad filter implementation
  - Resonance/Q control
  - Cutoff frequency control
  - More filter types (band-pass, notch)

### Stereo Sample Support
- **Status**: Beyond M5
- **Reason**: Mono-only for M4 specification
- **Current State**: Mono-only playback
- **Deferred**:
  - Stereo sample loading
  - Stereo-to-mono downmix option
  - Independent left/right panning

## Later Milestones

### Sample Editing
- **Status**: Beyond M5
- **Reason**: Not in current scope
- **Deferred**: Reverse, normalize, fade in/out

### Sample Library
- **Status**: M6
- **Reason**: Requires persistent storage (deferred per spec)
- **Deferred**: 
  - Sample database (SQLite/Room)
  - Tagging/categorization
  - Search/filter functionality
  - Filesystem storage layout

### Advanced Voice Management
- **Status**: Beyond M5
- **Reason**: Basic polyphony sufficient for M4
- **Deferred**:
  - Voice priority settings
  - Voice stealing strategies (newest-first, etc.)
  - Per-part voice count limits

### Sample Mapping
- **Status**: M4.5
- **Reason**: Current implementation auto-assigns to first empty part
- **Current State**: Auto-assignment to first empty part
- **Deferred**: 
  - Manual part selection
  - Drag-and-drop sample to part
  - Sample library with drag support