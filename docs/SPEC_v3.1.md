# TribeX (APK) – System Specification (v3.1)
Version: 3.1 (FINAL)
Status: Frozen / Development Ready
Platform: Android Native (Kotlin UI + C++ Audio Engine via Oboe)

## 0. Doktrin & Leitbild
TribeX ist eine Groovebox, keine DAW.
Die Entwicklung folgt strikt diesen Regeln:
 * Immediacy over Complexity: Wenn ein Feature drei Klicks braucht, ist es falsch implementiert.
 * Hardware-Feel: Bedienung erfolgt über Muskelgedächtnis (Positionen), nicht über Lesen von Texten.
 * Determinismus: Ein Pattern klingt live exakt so wie im Export. Keine Zufälle, die nicht durch einen Seed reproduzierbar sind.
 * Audio Authority: Der Audio-Thread hat absolute Priorität. UI ist nur eine Visualisierung des Audio-Zustands.
 * Offline First: Das Gerät ist ein Instrument. Kein Internet, kein Account, keine Cloud.

## 1. System-Architektur
### 1.1 Tech Stack
 * Language: C++17 (Audio Core), Kotlin (UI/Persistence).
 * Audio API: Oboe (AAudio mit OpenSL ES Fallback).
 * Data Bridge: JNI mit FlatBuffers/DirectByteBuffers (Zero-Copy wo möglich).
 * Persistence: Room (SQLite) für Metadaten, Filesystem für Samples.

### 1.2 Threading-Modell (Verbindlich)
 * Audio Thread: Realtime-Critical. Führt render() aus. Verbot: Allocations (new, malloc), System Calls, Locks (mutex), IO.
 * Control Thread: Nimmt UI-Events entgegen, schreibt in Lock-Free Queues für den Audio Thread.
 * Export/IO Thread: Führt render() für Offline-Export aus, lädt/speichert Samples.
 * UI Thread (Main): Zeichnet Interface. Darf niemals auf Audio warten.

## 2. Funktionsumfang (Hard Scope)
### 2.1 In Scope (Muss geliefert werden)
 * Sequencer: 16-64 Steps, Paging, Microtiming, Probability.
 * Sound Engines: 8x Sample Drum Parts, 1x Wavetable Synth Part (Chord-Mode).
 * FX: Delay, Reverb (Send), Valve Saturation, Limiter (Master).
 * Workflow: Pattern-Chain, Mute/Solo, Fill, Parameter Locks, Motion Rec.
 * IO: Sample Import (WAV), Resampling (Internal), Render Chain Export (WAV).

### 2.2 Out of Scope (Verboten)
 * Piano Roll / MIDI Editor.
 * Timeline / Song-Arranger View.
 * Plugin Support (VST/AU).
 * MIDI In/Out (Hardware).
 * Bluetooth Audio Support (Wegen Latenz aktiv abzuraten).
 * User Accounts / Cloud / Social Sharing.

## 3. User Experience (UX)
### 3.1 Navigations-Struktur
Das System besteht aus exakt 3 Screens. Keine Sub-Menüs.
 * PATTERN: Sequencer, Performance (Mute/Fill), Grid.
 * SOUND: ADSR, Pitch, Filter, FX-Sends, Master-FX.
 * SAMPLE: Browser, Edit (Trim), Assign, Resample.

### 3.2 Interaktions-Mechaniken
 * Sticky Shift: Button Toggle (Tap = ON, Action = Exec + OFF). Timeout: 3000ms. Kein Hold-Zwang.
 * Context Locks:
   * Trigger: Step halten.
   * State: UI wechselt visuell in "Lock Mode" (Rote Rahmen).
   * Action: Regler bewegen schreibt Wert in Step-Lock-Map.
   * Release: Rückkehr zu Global State.
 * Velocity Gesten:
   * Tap = Normal (80).
   * Swipe Up = Accent (127).
   * Swipe Down = Ghost (40).

### 3.3 Visualisierung
 * Lauflicht: Muss 60fps stabil sein. Realisierung via Canvas (Low-Level Drawing), keine Compose-Tree Updates pro Step.
 * Feedback: UI zeigt Audio-State, nicht Input-State. (Wenn Audio spielt, leuchtet Step. Nicht wenn User drückt).

## 4. Audio Engine Spezifikation
### 4.1 Die Render-Methode
Es existiert eine zentrale Methode zur Audio-Erzeugung.
```cpp
void AudioEngine::render(float* outputBuffer, int32_t numFrames) {
    // 1. Events aus Lock-Free Queue holen
    // 2. Sequencer Step-Logik (Sample-Accurate)
    // 3. Voices summieren
    // 4. FX Chain (Send -> Master -> Valve -> Limiter)
}
```

 * Live: Oboe ruft render() im Callback.
 * Export: Background-Thread ruft render() in while-Loop.

### 4.2 Latenz-Management
 * Buffer: FramesPerBurst * 2 (Double Buffering).
 * Burst: Audio wird in Bursts berechnet.
 * Timing: Sequencer nutzt currentSampleCount (int64), keine Wall-Clock.

### 4.3 Performance Degradation System
Automatische Anpassung bei XRun-Erkennung (Buffer Underruns).
 * Level 0 (Optimal): Max 24 Voices, Reverb High.
 * Level 1 (Warnung): Max 16 Voices, Reverb Low Density.
 * Level 2 (Kritisch): Max 8 Voices, Reverb OFF, Valve Bypass.

## 5. Sound Generation
### 5.1 Drum Sampler (8 Parts)
 * Polyphonie: Max 4 Stimmen pro Part. Stealing: Oldest.
 * Resampling: Linear Interpolation (Performance).
 * Parameter: Pitch, Pan, Level, Decay (AHD), Filter (1-Knob DJ-Style LP/HP).

### 5.2 Synth Part (1 Part)
 * Typ: Wavetable Monosynth.
 * Oszillator: Liest aus Pre-Calculated Wavetables.
 * Tables: Saw, Square, Sine, Chord-Major, Chord-Minor, Chord-7th.
 * Vorteil: Akkorde kosten exakt so viel CPU wie eine Mono-Stimme.
 * Filter: State Variable Filter (Resonant LP).

### 5.3 Master FX Chain
 * Valve Saturation:
   * Asymmetrischer Waveshaper (Tanh-Approximation).
   * Pre-Emphasis / De-Emphasis EQ.
   * Kein Oversampling (aus Performance-Gründen).
 * Limiter:
   * Lookahead: 1.5ms.
   * Hard-Ceiling: -0.3 dB.

## 6. Sequencer Logik
### 6.1 Datenmodell
 * Step: Gate (Bit), Velocity (2 Bit), Microtiming (Int8), Probability (Int8), Locks (Sparse Map).
 * Pattern: 1-4 Pages (16-64 Steps).
 * Chain: Liste von Pattern-IDs + Repeat-Count.

### 6.2 Determinismus (Probability)
 * Probability wird nicht zufällig gewürfelt.
 * RNG basiert auf Seed: Hash(PatternSeed + StepIndex + LoopIteration).
 * Garantie: Export = Live Playback.

### 6.3 Motion Recording
 * Aufnahme von Reglerbewegungen wird auf Steps quantisiert.
 * Wiedergabe: Wert gilt pro Step (Step & Hold). Keine Kurven-Interpolation in v3.1.

## 7. Sampling & IO
### 7.1 Resampling
 * Input: Interner Master-Bus (Post-Valve, Pre-Limiter).
 * Trigger: Sync zum Pattern-Start oder Free-Run.
 * Auto-Assign: Neues Sample wird automatisch auf den gewählten Drum-Part gemappt.

### 7.2 Export (Render Chain)
 * Rendert die aktive Pattern-Chain.
 * Geschwindigkeit: Offline (schneller als Echtzeit).
 * Format: WAV 16bit / 44.1kHz (oder Device Native).
 * Keine Normalisierung (Limiter regelt Ceiling).

## 8. Persistenz
### 8.1 Projekt-Format
 * Ein Ordner pro Projekt.
 * project.db (SQLite): Alle Pattern-Daten, Settings, Parameter.
 * /samples/: Ordner mit WAV-Dateien.

### 8.2 Save-Strategie
 * Atomic Write: DB wird in temporäre Datei geschrieben, dann atomar umbenannt.
 * Autosave: Bei Screen-Wechsel oder App-Pause.

---
End of Specification v3.1
