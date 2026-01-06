# TribeX – Project Rules (Frozen Spec v3.1)

## Non-Negotiables (Doc of Truth)
- Source of truth: `docs/SPEC_v3.1.md` (do not reinterpret).
- TribeX is a Groovebox, NOT a DAW.
- Offline first: no accounts, no cloud, no network features.
- Exactly 3 screens only: PATTERN, SOUND, SAMPLE. No submenus.

## Hard Out-of-Scope (Forbidden)
- Piano roll / MIDI editor
- Timeline / song arranger
- Plugins (VST/AU)
- MIDI hardware IO
- Bluetooth audio
- Accounts / cloud / social sharing

## Realtime Audio Rules (Mandatory)
Audio callback / render() must never do:
- memory allocations (new/malloc/realloc, STL growth)
- locks (mutex, std::lock_guard)
- blocking calls (sleep, futures waiting)
- I/O (file, database, logging spam)
- system calls that can block

If uncertain: treat as forbidden and refactor to Control/IO thread.

## Threading Model (Mandatory)
- Audio Thread: realtime-critical render callback (no waiting, no locks)
- Control Thread: receives UI events, writes to lock-free queues for audio
- Export/IO Thread: offline export render loop + sample load/store
- UI thread: renders UI; must never wait on audio

## Determinism (Mandatory)
- Live playback and offline export must produce identical results given same:
  - PatternSeed + pattern data + loop iteration count
- Probability is deterministic via seed-hash, not random runtime RNG.
- Sequencer timing uses sampleCounter (int64), not wall-clock.

## Implementation Standards
- No TODO placeholders. If something is intentionally deferred, document it in `docs/DEFERRED.md` with a clear reason.
- Every milestone must end with:
  1) build success
  2) tests green (unit tests at minimum)
  3) short notes in `docs/IMPLEMENTATION_NOTES.md`

## Output Format Rules (for the agent)
When writing files:
- Always output full file contents for every changed/new file.
- Prefer small, reviewable increments over massive rewrites.
- Never introduce new dependencies without a written justification in `docs/DECISIONS.md`.

## Safety / Secrets
- Never write API keys, keystores, local.properties, or credentials into the repo.
- If secrets are needed, reference env vars only.
