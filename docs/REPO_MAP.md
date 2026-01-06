# TribeX – Repository Structure

```
Tribex/
├── app/
│   ├── src/main/
│   │   ├── java/com/tribex/groovebox/   # Kotlin UI code
│   │   │   ├── ui/                       # Compose screens (PATTERN/SOUND/SAMPLE)
│   │   │   ├── viewmodel/                # ViewModels + StateFlow
│   │   │   └── data/                     # Room entities + DAOs
│   │   ├── cpp/                          # C++ audio engine
│   │   │   ├── AudioEngine.cpp           # Core render() method
│   │   │   ├── Sequencer.cpp             # Step logic
│   │   │   ├── Sampler.cpp               # Drum playback
│   │   │   └── native-lib.cpp            # JNI bridge
│   │   └── res/                          # Android resources
│   └── build.gradle.kts
├── docs/
│   ├── SPEC_v3.1.md                      # Frozen specification
│   ├── DECISIONS.md                      # Tech decisions (binding)
│   ├── ACCEPTANCE.md                     # Hard acceptance tests
│   ├── DEFERRED.md                       # Milestone roadmap
│   ├── M0_DEFINITION.md                  # M0 scope definition
│   └── CONTRIBUTING_AGENT.md             # Agent workflow rules
├── .clinerules/
│   ├── 01-tribex.md                      # Project rules for agents
│   └── workflows/
│       └── m0-scaffold.md                # M0 workflow
├── AGENTS.md                             # Entry point for AI agents
├── .clineignore                          # Files to exclude from agent context
└── .gitignore
```

**Key directories (once scaffolded):**
- `app/src/main/cpp/` – All C++ audio code (Oboe, DSP)
- `app/src/main/java/.../ui/` – Kotlin Compose UI
- `docs/` – All documentation, specs, decisions
