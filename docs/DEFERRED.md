# TribeX Deferred Features

This document tracks features that have been intentionally deferred to future milestones. Each entry must include:
- Description of feature
- Reason for deferral
- Target milestone (if known)

## QA AUDIT Deferred Items (08.01.2026)

### SAMPLE Screen Full Implementation
- **Description**: SampleScreen.kt contains only placeholder text, no functional implementation
  - No sample browser
  - No sample loading from storage
  - No sample metadata display
  - No waveform preview
  - No trim/loop points
- **Reason**: Per project QA rules - no new features in QA phase, only bugfixes and stability
- **Target**: M7 (per SPEC v3.1 requirements)

### Master FX Section UI Implementation
- **Description**: Master FX Section in SoundScreen.kt is placeholder only
  - No parameter controls for Valve Saturation, Limiter, Delay, Reverb
  - Placeholder text: "(M5: Placeholder - will be implemented in future milestone)"
- **Reason**: Per project QA rules - no new features in QA phase
- **Target**: M7

### Pattern Running Light Synchronization
- **Description**: Pattern running light uses System.currentTimeMillis() instead of audio state
  - Current implementation: `val currentStep = (System.currentTimeMillis() / 250).toUInt() % 16u`
  - Spec violation: "UI shows Audio-State, not Input-State"
  - Running light may drift from actual sequencer position
- **Reason**: Requires JNI methods for sequencer state polling (getCurrentStep, isPlaying, etc.)
- **Target**: M7 (requires JNI bridge implementation)

### KSP Version Upgrade
- **Description**: KSP version 1.9.20-1.0.14 incompatible with Kotlin 1.9.22
  - Build succeeds with warnings
  - Should upgrade to 1.9.22-1.0.18 or newer
- **Reason**: Non-blocking issue, not a critical bug
- **Target**: M7 (maintenance update)

### Compose Divider API Update
- **Description**: Divider deprecated in favor of HorizontalDivider
  - Used in: SampleBrowser.kt, SampleScreen.kt, PatternScreen.kt, SoundScreen.kt
  - Functional but deprecated
- **Reason**: Non-blocking issue, API still works
- **Target**: M7 (maintenance update)

## M8 Deferred Items

### Pattern/Part Persistence to Room
- **Description**: UI doesn't read/write patterns and part settings from Room
  - PATTERN screen uses in-memory state only
  - SOUND screen doesn't save parameters to Room
  - SAMPLE screen doesn't save samples to Room + filesystem
- **Reason**: M8 focused on persistence infrastructure (Room, ProjectManager)
- **Target**: M9 (UI Integration)

### Sample File Persistence
- **Description**: Sample files are saved to filesystem but not linked to Room
  - Audio data exists only in memory (AudioEngine)
  - No WAV file creation in project samples/ directory
  - No Sample entity insertion
- **Reason**: M8 focused on database infrastructure; sample I/O integration deferred
- **Target**: M9 (UI Integration)

### Versioned Room Migrations
- **Description**: Only destructive migration (fallbackToDestructiveMigration) implemented
  - Schema changes will delete all data
  - No incremental migration support
- **Reason**: Simple for M8; versioned migrations require more complex logic
- **Target**: Later milestone (when schema stabilizes)

### Project Selection UI
- **Description**: No UI for selecting/loading projects
  - Always loads last project or creates new one
  - No project list screen
  - No project creation dialog
- **Reason**: M8 focused on backend infrastructure
- **Target**: M9 (UI Integration)

### Project Export/Import
- **Description**: No project export/import functionality
  - Can't share projects with other devices
  - Can't backup/restore projects
- **Reason**: M8 focused on local persistence only
- **Target**: Later milestone (maybe M10 or after)

### Project Backup/Restore
- **Description**: No backup functionality
  - Can't create project backups
  - No automatic backup system
- **Reason**: M8 focused on basic persistence
- **Target**: Later milestone

### Room Schema Export
- **Description**: Schema export configured but not used
  - `room.schemaLocation` set to `$projectDir/schemas`
  - Schema files not committed to repo
  - Not used for migration testing
- **Reason**: Schema tracking useful but not critical for M8
- **Target**: M9 or later (when migrations are implemented)

### Pattern Steps Optimization
- **Description**: Pattern steps stored as JSON string
  - `steps: String = "{}"`
  - Parsing overhead on load
  - Not type-safe
- **Reason**: Simple for M8; future optimization with custom converters or serialization
- **Target**: M9 or later

### Waveform Blob Storage
- **Description**: Waveform data stored as ByteArray blob
  - Max 1000 points (4KB)
  - Stored in database (not filesystem)
  - May cause database bloat with many samples
- **Reason**: Simple for M8; could optimize to filesystem storage
- **Target**: Later milestone (performance optimization)


## M7 Deferred Items

### JNI Bridge for Export
- **Description**: Export JNI methods not implemented
  - `Java_com_tribex_groovebox_engine_AudioEngineBridge_startExport()`
  - `Java_com_tribex_groovebox_engine_AudioEngineBridge_stopExport()`
  - `Java_com_tribex_groovebox_engine_AudioEngineBridge_getExportProgress()`
  - File path handling via JNI strings
- **Reason**: M7 scope focused on C++ implementation only
- **Target**: M8 or later

### Kotlin Export UI
- **Description**: Export UI not implemented
  - Export button in PatternScreen or SoundScreen
  - File selection via Storage Access Framework
  - Progress display (0% to 100%)
  - Export status (Running, Success, Error)
  - Cancel export functionality
- **Reason**: UI work deferred to keep M7 focused on audio engine
- **Target**: M8 or later

### Pattern Chain Duration Calculation
- **Description**: Not implemented
  - Calculate total frames from active pattern chain
  - Consider repeat counts
  - Consider BPM
  - Currently using placeholder 60 seconds
- **Reason**: Requires pattern chain data structure and management UI
- **Target**: M8 or later

### Export Cancel
- **Description**: Stop functionality exists but UI not connected
  - `stopExport()` implemented in C++
  - Cancel button not added to UI
  - Progress callback not wired to UI
- **Reason**: UI work deferred
- **Target**: M8 or later

### Progress Reporting
- **Description**: Callback exists but UI integration missing
  - Progress callback implemented in OfflineRenderer
  - StateFlow updates not implemented in Kotlin
- **Reason**: UI work deferred
- **Target**: M8 or later

### File Path Management
- **Description**: User file selection not implemented
  - Export to app-internal storage (temporary)
  - Storage Access Framework integration deferred
  - User-specified filename not supported
- **Reason**: File I/O UI work deferred
- **Target**: M8 or later

## M4.5 Deferred Items

### Trim Persistence
- **Description**: Trim values (startOffset/endOffset) are not persisted to storage
- **Reason**: M4.5 focused on UI implementation; persistence is part of M6 (Project Save/Load)
- **Target**: M6

### Trim JNI Bridge
- **Description**: Trim updates from UI are not sent to AudioEngine via JNI
- **Reason**: M4.5 implemented UI-only trim; full integration requires JNI bridge updates
- **Target**: M4.6 (hotfix) or M5

### Waveform Cache
- **Description**: Waveform data is recalculated on each sample reload
- **Reason**: Acceptable for M4.5; caching would add complexity
- **Target**: M6 (Project Save/Load with sample metadata caching)

### Trim Slider Touch Handling
- **Description**: TrimSlider component is visual-only using RangeSlider
- **Reason**: Full touch handling with drag gestures would require custom implementation
- **Target**: M5 (UI polish) or later

### Sample Preview Playback
- **Description**: No audio preview when browsing samples
- **Reason**: Requires additional voice management and UI integration
- **Target**: M5 or later

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
- **Status**: Beyond M4.5
- **Reason**: Current implementation auto-assigns to first empty part
- **Current State**: Auto-assignment to first empty part
- **Deferred**: 
  - Manual part selection
  - Drag-and-drop sample to part
  - Sample library with drag support