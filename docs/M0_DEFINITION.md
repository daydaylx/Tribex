# M0 – Definition of Done

## In Scope (must deliver)

1. **Project scaffold**
   - Android project with Kotlin + CMake/NDK
   - Gradle config per docs/DECISIONS.md (minSdk 24, targetSdk 34, NDK 26.1.x)
   - Oboe dependency wired

2. **Minimal audio engine**
   - `AudioEngine.cpp` with `render()` generating 440Hz sine wave
   - JNI bridge (`native-lib.cpp`)
   - Proper start/stop lifecycle (no audio leak on pause)

3. **Minimal UI**
   - Single screen with Start/Stop button
   - Visual feedback (button state reflects audio state)

4. **Verification**
   - `./gradlew :app:assembleDebug` succeeds
   - App installs and launches on device/emulator
   - Tone plays stable for 30-60 seconds without glitches
   - Start/Stop toggles audio reliably

## Out of Scope (do NOT implement in M0)

- Sequencer / step grid
- Sample loading / playback
- Room database / persistence
- Multiple screens (PATTERN/SOUND/SAMPLE)
- Any FX (delay, reverb, etc.)
- Any UI beyond Start/Stop button
- Pattern chains
- MIDI / export / import

## Acceptance Gate

M0 is complete when:
- [ ] Debug APK builds without errors
- [ ] 440Hz tone plays for 60s without dropout
- [ ] Start/Stop works 10 times in a row without crash
- [ ] docs/IMPLEMENTATION_NOTES.md updated with test device info
