# TribeX – Acceptance Criteria

These are hard acceptance tests. A milestone is not complete until all relevant criteria pass.

## Audio Stability

- [ ] **Sine/Click 60s**: App plays a 440Hz sine or click track for 60 seconds without audible glitches (no XRuns logged)
- [ ] **Background resilience**: Audio continues without dropout when switching to another app briefly
- [ ] **Cold start latency**: First sound plays within 500ms of user tap

## Determinism

- [ ] **Export == Live**: Given identical PatternSeed + pattern data + loop count, offline export produces bit-identical output to recorded live playback
- [ ] **Probability reproducibility**: Running same pattern 10 times with same seed produces identical trigger decisions

## Threading Safety

- [ ] **UI never blocks audio**: Scrolling UI, loading samples, or saving project does not cause audio dropout
- [ ] **No allocations in render()**: Static analysis or runtime profiling confirms zero heap allocations in audio callback
- [ ] **No locks in render()**: Code review confirms no mutex/lock usage in audio thread path

## Performance

- [ ] **60fps step indicator**: Sequencer position light updates at 60fps during playback (measure via systrace)
- [ ] **8 voices + FX**: 8 simultaneous drum voices + delay + reverb + valve + limiter runs without XRun on target device

## UX

- [ ] **3 screens only**: Navigation structure contains exactly PATTERN, SOUND, SAMPLE - no hidden menus
- [ ] **No network calls**: App functions fully in airplane mode; no analytics, no crash reporting that requires network

## Build Health

- [ ] **assembleDebug**: `./gradlew :app:assembleDebug` completes without errors
- [ ] **Unit tests green**: `./gradlew test` passes all tests
- [ ] **No secrets in repo**: `local.properties`, `*.jks`, `*.keystore` are gitignored and not committed
