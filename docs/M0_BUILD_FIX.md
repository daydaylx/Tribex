# M0 Build Fix - Reproduktionsschritte

## Zusammenfassung

Alle Build-Probleme wurden behoben. Die Konfiguration ist jetzt auf einen stabilen, getesteten Version Stack eingestellt.

## Geänderte Dateien

### 1. build.gradle.kts
- AGP: 8.2.0 → 8.0.2
- Kotlin: 1.9.20 → 1.8.22
- Deprecated `buildDir` entfernt

### 2. app/build.gradle.kts
- Syntaxfehler in Zeile 48 korrigiert (doppelte Klammer)
- Compose Compiler: 1.5.4 → 1.4.6 (Kompatibilität zu Kotlin 1.8.22)

### 3. gradle/wrapper/gradle-wrapper.properties
- Gradle: 8.2 → 8.0

### 4. docs/DECISIONS.md
- Alle Versionen dokumentiert

### 5. docs/IMPLEMENTATION_NOTES.md
- Build-Probleme und Lösungen dokumentiert

## Exakte Reproduktionsschritte

### Schritt 1: Projekt sauber aufsetzen

```bash
# Verzeichnis bereinigen
cd /home/d/Schreibtisch/Tribex
rm -rf build app/build .gradle

# Gradle Wrapper generieren (benötigt Android Studio oder gradle CLI)
gradle wrapper --gradle-version 8.0
```

**HINWEIS:** Wenn `gradle wrapper` fehlschlägt, verwende Android Studio:
1. Öffne das Projekt in Android Studio
2. Android Studio generiert automatisch den Gradle Wrapper
3. Oder: File → Sync Project with Gradle Files

### Schritt 2: Build ausführen

```bash
# Debug Build
./gradlew :app:assembleDebug

# Mit Stacktrace für Fehlerdiagnose
./gradlew :app:assembleDebug --stacktrace

# Oder mit --info für detaillierte Logs
./gradlew :app:assembleDebug --info
```

### Schritt 3: APK installieren und testen

```bash
# Auf Device/Emulator installieren
./gradlew :app:installDebug

# APK liegt hier:
ls -lh app/build/outputs/apk/debug/app-debug.apk
```

### Schritt 4: M0 Tests durchführen

1. App starten
2. "Start Audio" Button drücken
3. 10x Start/Stop testen (schnelles Wechseln)
4. 60s Playback auf Glitches prüfen (klacken, rauschen)

## Bekannte Workarounds

### Wenn Gradle Wrapper nicht generiert werden kann

**Option A: Mit Android Studio**
```bash
# Projekt in Android Studio öffnen
studio.sh /home/d/Schreibtisch/Tribex
# Warten bis Gradle Sync abgeschlossen ist
# Dann Build → Make Project
```

**Option B: Manuelle Dateien (nicht empfohlen)**
```bash
# Falls du Zugriff auf Gradle 8.0 hast, kopiere diese Dateien:
# - gradlew (chmod +x)
# - gradlew.bat (für Windows)
# - gradle/wrapper/gradle-wrapper.jar
```

**Option C: Alternative Gradle CLI**
```bash
# Falls gradle CLI verfügbar:
gradle --version
gradle :app:assembleDebug
```

## Erfolgreicher Build

Der Build ist erfolgreich, wenn:

✓ `BUILD SUCCESSFUL` in der Konsole
✓ `app/build/outputs/apk/debug/app-debug.apk` existiert
✓ Keine C++ Linking Fehler
✓ Keine JNI Fehler
✓ Keine Compose Compiler Fehler

## Fehlerdiagnose

### HasConvention Fehler
→ Versionen in DECISIONS.md überprüfen, alle Dateien müssen konsistent sein

### module() Fehler
→ Gradle Version mit AGP Version abgleichen (DECISIONS.md)

### JNI/Symbol not found
→ CMakeLists.txt prüfen, Oboe linking korrekt?

### Compose Compiler Fehler
→ Kotlin und Compose Compiler Version abgleichen (DECISIONS.md)

## Nächste Schritte nach erfolgreichem Build

1. App auf Device installieren
2. M0 Tests durchführen (siehe oben)
3. Bei Erfolg → M1 starten

## Dokumentation

Alle Entscheidungen sind dokumentiert in:
- `docs/DECISIONS.md` - Versionen und Rationale
- `docs/IMPLEMENTATION_NOTES.md` - Build-Probleme und Lösungen