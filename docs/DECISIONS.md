# TribeX – Design Decisions

This document records all binding technical decisions. Agents must not deviate without updating this file first.

## Build Configuration

| Decision | Value | Rationale |
|----------|-------|-----------|
| applicationId | `com.tribex.groovebox` | Confirmed for M0 |
| minSdk | 24 (Android 7.0) | Oboe AAudio support starts at API 26, but OpenSL ES fallback covers 24+ |
| targetSdk | 34 | Current stable |
| compileSdk | 34 | Match target |
| NDK Version | `26.1.10909125` | LTS, tested with Oboe 1.8 |
| ABIs | `arm64-v8a` | Start mono-ABI; add `armeabi-v7a` only if needed |
| CMake Version | `3.22.1` | Bundled with Android Studio |
| Android Gradle Plugin | `8.0.2` | Tested stable stack, compatibility fixes |
| Kotlin | `1.8.22` | Matches AGP 8.0.2 |
| Compose Compiler | `1.4.6` | Matches Kotlin 1.8.22 |
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
| Room | 2.6.x | SQLite abstraction for pattern/project persistence |
| Kotlin | 1.9.x | Current stable for Android |
| Compose | 1.5.x | Modern UI toolkit (evaluate Canvas performance) |

## Open Decisions (to be resolved)

- [ ] Compose vs View-based UI for 60fps step indicator (needs profiling)
- [ ] FlatBuffers vs DirectByteBuffer for JNI bridge (evaluate complexity vs performance)