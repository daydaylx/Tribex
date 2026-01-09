# NEXTSTEP

## 0. TL;DR
- **CI/CD fehlt komplett:** Keine GitHub Actions, keine automatischen Checks.
- **Audio-Thread Safety:** Logging (`__android_log_print`) im Audio-Callback (P0).
- **Memory Safety:** Race Condition in `SamplePart` (`mRetiredSamples`) (P0).
- **Debug-Code in Production:** "Agent Log" schreibt bei jedem Pattern-Load in Datei (P0).
- **Testing:** Keine Android Instrumentation Tests (`androidTest` fehlt).
- **Quality:** Kein Linter/Formatter (Detekt/Ktlint) konfiguriert.

## 1. Repo-Überblick
- **Zweck:** Offline-first Android Groovebox (Pattern/Sound/Sample), deterministischer Audio-Engine.
- **Tech Stack:** Android (Kotlin, Compose, Room), C++17 (Oboe), CMake.
- **Start:**
  - `./gradlew :app:assembleDebug` (Build)
  - `./gradlew test` (Unit Tests)
  - `./gradlew nativeTest` (C++ Tests via CMake/CTest)
  - App auf Emulator/Device mit minSdk 26+ deployen.

## 2. Kritische Probleme (P0)

### 1. Blocking I/O & Logging im Audio-Path
- **Impact:** Audio-Glitches, Dropouts, Performance-Einbrüche. Violation der Realtime-Constraints.
- **Evidence:** 
  - `app/src/main/cpp/AudioEngine.cpp:218`: `LOGE` (`__android_log_print`) in `processEvents` (wird von `onAudioReady` gerufen).
  - `app/src/main/cpp/native-lib.cpp`: Massive `#region agent log` Blöcke mit `fopen`/`fprintf` in JNI-Calls.
- **Fix:**
  1. Alle `LOGE`/`LOGI` aus `AudioEngine::processEvents` und `onAudioReady` entfernen.
  2. Stattdessen `std::atomic<int> errorCounter` hochzählen und vom UI-Thread pollen.
  3. "Agent Log" Code (`fopen` in `native-lib.cpp`) komplett entfernen.
- **Aufwand:** S (1 Tag)

### 2. Race Condition in `SamplePart` Memory Management
- **Impact:** Potential Crash (Segfault) oder Memory Corruption beim Laden von Samples während Playback.
- **Evidence:** `app/src/main/cpp/SamplePart.cpp:33`: `mRetiredSamples.push_back()` (Alloc) passiert im Loading-Thread. `releaseRetiredSamples` leert diesen Vector im Stop-Thread. Kein Mutex. `std::vector` ist nicht thread-safe.
- **Fix:**
  1. `std::mutex` für `mRetiredSamples` Zugriff einführen (nur im Loading/Main-Thread, nie im Audio-Thread!).
  2. Sicherstellen, dass `releaseRetiredSamples` nicht während `loadSample` läuft (Logik prüfen).
- **Aufwand:** M (2 Tage)

### 3. CI/CD Pipeline fehlt
- **Impact:** Keine Garantie, dass Builds funktionieren oder Tests passen. "Works on my machine"-Risiko.
- **Evidence:** Kein `.github/` Folder.
- **Fix:**
  1. `.github/workflows/android.yml` anlegen.
  2. Jobs: Build (Debug/Release), Unit Test (JUnit), Native Test (CTest), Lint.
- **Aufwand:** S (1 Tag)

## 3. Wichtige Probleme (P1)

### 1. Fehlende Instrumentation Tests
- **Impact:** UI-Logik, Datenbank-Migrationen und JNI-Integration ungetestet auf echtem Android-OS.
- **Evidence:** Ordner `app/src/androidTest` existiert nicht.
- **Fix:**
  1. Ordner anlegen.
  2. Ersten Test schreiben: `ProjectDatabaseTest` (Migrationen, DAO basics).
  3. UI-Test: Start App -> Check ob PatternScreen lädt.
- **Aufwand:** M (3 Tage)

### 2. NDK Version Pinning
- **Impact:** Nicht-reproduzierbare Builds, da Default-NDK variieren kann.
- **Evidence:** `app/build.gradle.kts` hat `ndk { abiFilters... }` aber keine `ndkVersion`.
- **Fix:** `android { ndkVersion = "26.1.10909125" }` (oder passende Version) setzen.
- **Aufwand:** S (10 Min)

### 3. Linting & Formatting
- **Impact:** Inkonsistenter Code-Stil, potentielle Bugs (z.B. ungenutzte Ressourcen).
- **Evidence:** Keine Configs für Detekt/Ktlint/Android Lint gefunden.
- **Fix:**
  1. Detekt Plugin in `build.gradle.kts` einbinden.
  2. Basis-Config (`detekt.yml`) anlegen.
  3. Pre-commit Hook oder CI Job einrichten.
- **Aufwand:** S (4 Stunden)

## 4. Verbesserungen (P2)

### 1. JNI Bridge Refactoring
- **Impact:** Wartbarkeit. Aktuell viel Boilerplate in `native-lib.cpp`.
- **Evidence:** `native-lib.cpp` mischt JNI-Logik, Business-Logik und Debug-Logging.
- **Fix:** JNI-Methoden auf das Nötigste reduzieren. Logging entfernen. Ggf. JavaCPP oder sauberere C++ Wrapper-Klasse nutzen.
- **Aufwand:** M

### 2. Strict Mode für Audio Thread
- **Impact:** Verhindert zukünftige Regressionen (Blocking Calls).
- **Fix:** Oboe `setPerformanceMode(LowLatency)` ist gesetzt, aber wir sollten Laufzeit-Checks (z.B. via Perfetto Tracing oder Time-Budget Checks) einbauen, die warnen, wenn der Callback zu lange dauert.
- **Aufwand:** L

## 5. Erweiterungsvorschläge (P3)

### 1. Automated Audio Glitch Detection
- **Nutzen:** QA für Audio-Engine.
- **Ansatz:** Im Unit-Test Audio rendern und auf Diskontinuitäten prüfen oder `AudioEngine::process()` in Loop laufen lassen und Zeit messen.
- **Aufwand:** M

### 2. Screenshot Tests
- **Nutzen:** Verhindert visuelle Regressionen im UI.
- **Ansatz:** Paparazzi oder Roborazzi für Compose Previews einbinden.
- **Aufwand:** M

## 6. Quick Wins (unter 2 Stunden)
1. **NDK Pinning:** `ndkVersion` in `app/build.gradle.kts` eintragen.
2. **Cleanup:** `AudioEngineBridge.setDebugLogPath` und den zugehörigen C++ Code entfernen (das File-Logging ist gefährlich).
3. **Docs:** `CONTRIBUTING.md` updaten mit Hinweis auf Code Style (Detekt).

## 7. Empfohlene Roadmap (2–6 Wochen)

- **Woche 1 (Stability & Safety):**
  - Logging aus Audio-Pfad entfernen (P0).
  - `SamplePart` Race Condition fixen (P0).
  - CI Pipeline (GitHub Actions) aufsetzen (P0).
  
- **Woche 2 (Quality Baseline):**
  - NDK pinnen (P1).
  - Detekt/Ktlint einrichten (P1).
  - `androidTest` Setup + erster DB-Test (P1).
  
- **Woche 3 (Cleanup & Refactor):**
  - `native-lib.cpp` aufräumen ("Agent Log" raus) (P2).
  - Test Coverage erhöhen (C++ & Kotlin).
  
- **Woche 4+ (Features):**
  - Beginnen mit M9 (Landscape) / M10 (Waveform) Tasks laut `DEFERRED.md`.

## 8. Optional: "Wenn du es wirklich ernst meinst"
- **Full Architecture Audit:** Der Mix aus JNI Calls und ViewModel State ist fragil. Überlegung: Eine reine C++ State-Machine, die das UI treibt ("Unidirectional Data Flow" bis runter in C++), statt UI -> ViewModel -> JNI -> AudioEngine. Das würde State-Sync-Probleme eliminieren.
