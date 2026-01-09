# TribeX  Deferred Features

This document tracks features intentionally deferred to future milestones. All deferred features must have a clear rationale and a planned milestone.

## M9: Landscape Orientation

### SAMPLE Screen - Deferred Features

**Waveform Preview**
- **Status**: Layout prepared, implementation deferred to M10
- **Reason**: Focus of M9 is landscape layout adaptation. Waveform rendering requires:
  - Native audio sample data access
  - Canvas-based waveform drawing at 60fps
  - Zoom/pan gesture handling
  - Performance optimization for large samples
- **Planned Implementation**: M10 with Canvas-based waveform component

**Trim/Loop Controls**
- **Status**: UI placeholder in place, functionality deferred to M10
- **Reason**: Depends on waveform preview component. Trim requires:
  - Visual start/end markers on waveform
  - Touch drag interaction for trim points
  - Loop region visualization
  - Audio engine integration for trimmed playback
- **Planned Implementation**: M10 with interactive trim/loop UI

**Sample Metadata Editing**
- **Status**: Read-only display implemented
- **Reason**: Not critical for M9 layout focus. Deferred features:
  - Editable sample name
  - BPM detection/display
  - Key/root note assignment
  - One-shot vs loop mode toggle
- **Planned Implementation**: M11 or later

**Sample Import/Export**
- **Status**: Not implemented
- **Reason**: Requires file system access and sample loader integration:
  - File picker for importing external samples
  - Export sample to device storage
  - Sample format conversion (if needed)
  - Error handling for invalid formats
- **Planned Implementation**: M11 or later

## Previous Deferred Items (Archived)

### M8: Persistence (Implemented)
- [x] Project save/load - Implemented in M8
- [x] Autosave on screen change - Implemented in M8
- [x] Autosave on app pause/background - Implemented in M8
- [x] SQLite database with Room - Implemented in M8
- [x] Atomic save with temp backup - Implemented in M8

### M7: Offline Export (Implemented)
- [x] C++ offline renderer - Implemented in M7
- [x] WAV export at 44.1kHz/16-bit - Implemented in M7
- [x] Determinism (Export == Live) - Implemented in M7
- [x] JNI bridge for export - Deferred to M8 (UI integration)

### M6: FX Implementation (Implemented)
- [x] Delay Effect - Implemented in M6
- [x] Reverb Effect - Implemented in M6
- [x] Valve Saturation - Implemented in M6
- [x] Limiter - Implemented in M6
- [x] FX Manager - Implemented in M6

### M5: UI Structure (Implemented)
- [x] Navigation between screens - Implemented in M5
- [x] PATTERN Screen layout - Implemented in M5
- [x] SOUND Screen layout - Implemented in M5
- [x] SAMPLE Screen placeholder - Implemented in M5

### M4: Audio Integration (Implemented)
- [x] Part mute/solo - Implemented in M4
- [x] Part pan - Implemented in M4
- [x] Voice level per part - Implemented in M4
- [x] Voice decay per part - Implemented in M4
- [x] Filter per drum part - Implemented in M4

### M3: Pattern Screen (Implemented)
- [x] Step grid UI - Implemented in M3
- [x] Part selection - Implemented in M3
- [x] Page navigation (16-64 steps) - Implemented in M3
- [x] BPM display - Implemented in M3
- [x] Transport controls - Implemented in M3

## Future Milestone Planning

### M10: SAMPLE Screen Implementation
- Waveform preview component (Canvas-based)
- Trim/loop controls with visual markers
- Sample assignment to drum parts
- Sample metadata editing
- Basic sample import from device storage

### M11: Advanced SAMPLE Features
- Sample library management
- Sample export to device storage
- Sample format conversion
- BPM detection
- Key/root note assignment

### M12: Pattern Chain
- Chain multiple patterns
- Pattern transition controls
- Song length configuration

## Notes for Future Development

- **Performance Priority**: Waveform rendering must maintain 60fps. Consider caching or downsampling for large samples.
- **Touch Targets**: All trim/loop controls must be >= 44dp for accessibility.
- **Offline First**: No cloud features per SPEC v3.1.
- **No TODOs in Code**: All deferred items must be documented here, not as TODO comments.