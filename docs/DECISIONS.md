# TribeX – Design Decisions

This document records all binding technical decisions. Agents must not deviate without updating this file first.

## Build Configuration

| Decision | Value | Rationale |
|----------|-------|-----------|
| applicationId | `com.tribex.groovebox` | Confirmed for M0 |
| minSdk | 26 (Android 8.0) | M4.5: Updated from 24 for AAudio direct support (99%+ device coverage) |
| targetSdk | 34 | Current stable |
| compileSdk | 34 | Match target |
| NDK Version | `26.1.10909125` | LTS, tested with Oboe 1.8 |
| ABIs | `arm64-v8a` | Start mono-ABI; add `armeabi-v7a` only if needed |
| CMake Version | `3.22.1` | Bundled with Android Studio |
| Android Gradle Plugin | `8.0.2` | Tested stable stack, compatibility fixes |
| Kotlin | `1.9.22` | M4.5/M6: Updated from 1.8.22 for modern language features |
| Compose Compiler | `1.5.10` | M6: Updated for Kotlin 1.9.22 compatibility |
| Gradle | `8.0` | Matches AGP 8.0.2 |

## Audio Configuration

| Decision | Value | Rationale |
|----------|-------|-----------|
| Sample Rate (Live) | Device native | Avoid resampling; let Oboe pick optimal |
| Sample Rate (Export) | 44100 Hz | Standard CD quality, per spec |
| Bit Depth (Export) | 16-bit PCM | Per spec |
| Buffer Size | FramesPerBurst * 2 | Double-buffering for stability |
| Oboe Source | Gradle dependency | `implementation 'com.google.oboe:oboe:1.8.0'` |

## Dependencies

| Dependency | Version | Justification |
|------------|---------|---------------|
| Oboe | 1.8.0 | Official Google low-latency audio library |
| Room | 2.6.1 | M8: SQLite abstraction for pattern/project persistence |
| Room KTX | 2.6.1 | M8: Coroutine support for Room |
| Room Compiler (KSP) | 2.6.1 | M8: Annotation processing for Room |
| KSP Plugin | 1.9.20-1.0.14 | M8: Annotation processing (compatible with Kotlin 1.9.20) |
| Kotlin | 1.9.22 | M4.5/M6: Updated from 1.8.22 for modern features |
| Compose BOM | 2024.02.01 | M4.5: Updated for latest stable compose versions |
| Compose Compiler | 1.5.10 | M6: Compatibility with Kotlin 1.9.22 |
| Kotlin Serialization | 1.6.2 | M8: JSON serialization for pattern steps/parameters |
| Lifecycle Process | 2.7.0 | M8: ProcessLifecycleOwner for autosave on app background |
| Lifecycle ViewModel Compose | 2.7.0 | Compose integration for viewModel() usage |

## M7: Offline Export Decisions

| Decision | Value | Rationale |
|----------|-------|-----------|
| Export Sample Rate | 44.1 kHz fixed | CD quality, standard per spec |
| Export Bit Depth | 16-bit PCM | Per spec, good balance of quality/file size |
| Export Format | Stereo | All audio processing is stereo |
| Render Chunk Size | 1024 frames | Balanced speed/memory for offline rendering |
| Determinism Method | Exact same render() logic | Ensures Export == Live playback |
| Export Duration | Placeholder 60s | M7 focused on C++ implementation; pattern chain duration deferred to M8 |
| JNI Bridge | Not implemented | M7 scope: C++ only; UI/JNI deferred to M8 |
| Normalization | None (disabled) | Limiter regulates ceiling to -0.3 dB per spec |

## M8: Persistence Decisions

| Decision | Value | Rationale |
|----------|-------|-----------|
| Database Path | `/data/data/com.tribex.groovebox/files/projects/{projectUUID}/project.db` | One DB per project for isolation |
| Samples Path | `/data/data/com.tribex.groovebox/files/projects/{projectUUID}/samples/` | Co-located with project data |
| Exports Path | `/data/data/com.tribex.groovebox/files/exports/` | Separate from projects for easy management |
| Database Journaling | WAL (Write-Ahead Logging) | Better performance, automatic crash recovery |
| Atomic Save Strategy | Room transactions + temp file backup + verify | Extra layer of protection against corruption |
| Schema Versioning | Room Migrations (no destructive fallback) | Preserve user data across updates |
| Export Format JSON | Pattern data, settings, metadata (no WAVs) | Human-readable, manageable file size |
| Export Format CSV | Pattern matrix (Step × Parts) | Compatible with external tools |
| Backup Format | Directory copy (DB + samples) | Full project snapshot for restore |

## M8: File Structure

```
/data/data/com.tribex.groovebox/files/
├── projects/
│   ├── {projectUUID1}/
│   │   ├── project.db              # SQLite database (Room)
│   │   ├── project.db.tmp          # Temporary backup (atomic save)
│   │   ├── project.db-wal          # WAL file (auto-created)
│   │   └── samples/
│   │       ├── sample_part0.wav
│   │       ├── sample_part1.wav
│   │       └── ...
│   ├── {projectUUID2}/
│   │   └── ...
└── exports/
    ├── {projectName}_20260101_120000.json    # JSON export
    ├── {projectName}_pattern0_20260101_120000.csv  # CSV export
    └── {projectName}_backup_20260101_120000/ # Full backup
        ├── README.txt
        ├── project.db
        └── samples/
            └── ...
```

## M8: Persistence Workflow

1. **Project Creation**
   - Create UUID-based directory
   - Create `samples/` subdirectory
   - Initialize Room database with default data

2. **Project Loading**
   - Close previous database (if any)
   - Open new database with Room
   - Load project entity into memory
   - Update `last_project_id` preference

3. **Autosave Trigger**
   - Screen switch: Call `ProjectManager.autosave()`
   - App pause/background: Call `ProjectManager.autosave()`

4. **Atomic Save Process**
   - Update database via Room transactions
   - Update `lastModified` timestamp
   - Close database (flushes WAL)
   - Copy DB to `project.db.tmp`
   - Verify backup integrity
   - Reopen database

5. **Export/Backup**
   - JSON: Export metadata only (no WAVs)
   - CSV: Export pattern matrix for single pattern
   - Backup: Full directory copy (DB + samples)

## M8: Crash Recovery Strategy

- **WAL Mode**: Automatic recovery on next open
- **Temp Backup**: If WAL recovery fails, restore from `.tmp` file
- **Best Effort**: Last known good state may be lost if crash during critical section

## M9: Landscape Orientation

| Decision | Value | Rationale |
|----------|-------|-----------|
| Screen Orientation | `sensorLandscape` | Hardware groovebox feel;双手操作 |
| Landscape Type | Sensor (auto-rotate) | Allows both landscape orientations for flexible device placement |
| Layout Strategy | Row-based (2-column) | Left: Controls (220dp), Right: Content (remaining) |
| Pattern Screen | Split layout: Controls left, StepGrid right | StepGrid needs maximum space; controls accessible with one hand |
| Sound Screen | 2-column parameter layout | Parameters split across left/right to reduce scrolling |
| Sample Screen | Split view: Browser left, Details right | Preview and assignment visible simultaneously |
| Touch Targets | >= 44dp (already satisfied) | All controls remain accessible in landscape |
| StepGrid | Canvas-based (unchanged) | Maintain 60fps performance without full-screen recomposition |

### M9: Rationale for Landscape-Only

TribeX is a groovebox (hardware metaphor), not a DAW. Landscape orientation provides:

1. **Two-Thumb Operation**: Natural for holding device with both hands (like a physical groovebox)
2. **Step Grid Space**: 16-64 steps need horizontal space; vertical scrolling breaks the "beat grid" metaphor
3. **Hardware Feel**: Mirrors TR-808/909, MPC, etc. - all designed for wide displays
4. **Control Density**: More horizontal space for simultaneous access to transport, mute/solo, and pattern editing
5. **Performance View**: StepGrid + controls visible without scrolling (critical for live performance)

### M9: Screen-Specific Layout Decisions

**Pattern Screen**
- Left column (220dp): Transport, Shift, BPM, Page, Part Tabs, Mute/Solo
- Right column (flex): StepGrid (Canvas-based, maximum space)
- Bottom Navigation remains at bottom (Scaffold padding)

**Sound Screen**
- Left column: Part Selector, Rec Toggle, AMP/FILTER parameters, PITCH
- Right column: ADSR, FX SEND, Master FX
- Parameters grouped logically; no submenus (per SPEC v3.1)

**Sample Screen**
- Left column (flex): Sample browser list (LazyColumn)
- Right column (flex): Assignment controls, Waveform preview placeholder, Trim controls placeholder
- Deferred items (M10): Waveform rendering, Trim/Loop point editing documented in DEFERRED.md

## Open Decisions (to be resolved)

- [x] Compose vs View-based UI for 60fps step indicator (Canvas-based, implemented in StepGrid component)
- [ ] FlatBuffers vs DirectByteBuffer for JNI bridge (evaluate complexity vs performance)
- [ ] Export UI placement: PatternScreen vs SoundScreen vs new ExportScreen (M8)
