# M0 – Scaffold & Audio Smoke Test

## Goal
Establish a buildable Android project with minimal Oboe integration that can play a test tone.

## Prerequisites
- Android Studio installed
- NDK version as specified in docs/DECISIONS.md
- CMake available via SDK Manager

## Steps

1. **Project Setup**
   - Create Android project with Kotlin + C++ (CMake) support
   - Configure build.gradle with versions from docs/DECISIONS.md
   - Wire NDK and CMake in app/build.gradle

2. **Oboe Integration**
   - Add Oboe dependency to build.gradle
   - Create minimal `AudioEngine.cpp` with `render()` method
   - Implement JNI bridge (`native-lib.cpp`)

3. **Minimal Audio Engine**
   - `AudioEngine::render()` generates 440Hz sine wave
   - No sequencer logic yet, just continuous tone
   - Proper start/stop lifecycle

4. **UI Integration**
   - Single screen with Start/Stop button
   - Button calls JNI to start/stop audio engine
   - Visual feedback (button state change)

5. **Verification**
   - [ ] `./gradlew :app:assembleDebug` succeeds
   - [ ] App installs and launches
   - [ ] Tone plays for 30+ seconds without glitches
   - [ ] Start/Stop works reliably

6. **Documentation**
   - Update docs/IMPLEMENTATION_NOTES.md with:
     - Any deviations from plan
     - Device(s) tested on
     - Known issues (if any)

## Acceptance
This milestone is complete when:
- Build passes
- Tone plays stable for 30s
- Notes written to IMPLEMENTATION_NOTES.md
