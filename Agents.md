# AGENTS.md – TribeX (Android APK)

## Project summary
TribeX is an offline-first groovebox (not a DAW). Kotlin UI + C++17 audio engine via Oboe. Deterministic playback: live == export.

## Setup (local)
- Install Android Studio + SDK
- Install Android NDK (recommend a fixed version and keep it in docs/DECISIONS.md)
- Ensure CMake + Ninja are available via Android Studio SDK Manager

## Build commands
- Gradle tasks: `./gradlew tasks`
- Debug build: `./gradlew :app:assembleDebug`
- Unit tests: `./gradlew test`
- Instrumented tests (if present): `./gradlew connectedAndroidTest`

## Repo rules (must follow)
- Source of truth: docs/SPEC_v3.1.md
- Exactly 3 screens: PATTERN / SOUND / SAMPLE
- No cloud, no accounts, no bluetooth audio, no DAW features
- Audio callback must never allocate, lock, block, or do IO
- Control->Audio communication uses lock-free queues / atomics

## Code style
- Kotlin: keep UI state in ViewModels (StateFlow). UI must not block audio.
- C++: prefer fixed-size buffers, preallocation, and no dynamic allocation on audio thread.
- Keep JNI boundary minimal; use DirectByteBuffer where possible.

## Architecture notes
- AudioEngine::render() is the single source of audio generation.
- Offline export must call the same render() in a loop (determinism requirement).
- Samples stored in filesystem; metadata/patterns in SQLite (Room).

## Where to look first
- docs/SPEC_v3.1.md
- docs/DECISIONS.md
- docs/ACCEPTANCE.md
