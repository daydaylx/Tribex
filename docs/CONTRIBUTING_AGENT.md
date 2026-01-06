# Agent Contribution Guidelines

## Workflow

1. **Read first** – Before any change, read relevant docs (SPEC_v3.1.md, DECISIONS.md, M0_DEFINITION.md)
2. **Small steps** – One logical change per commit. Build must pass after each commit.
3. **Build gate** – Run `./gradlew :app:assembleDebug` before committing. Do not push broken builds.
4. **Document deviations** – If you deviate from DECISIONS.md, update the file with rationale.

## Commit Style

```
[M0] Add Oboe dependency to build.gradle

- Added com.google.oboe:oboe:1.8.0
- Configured CMake for NDK build
```

Prefix with milestone tag: `[M0]`, `[M1]`, etc.

## Forbidden

- `TODO` comments without immediate resolution
- Placeholder code ("implement later")
- Commits that break the build
- Changes outside current milestone scope
- Secrets, API keys, or sample files in repo

## File Updates

When making changes, update:
- `docs/IMPLEMENTATION_NOTES.md` – Deviations, test results, issues
- `docs/DECISIONS.md` – Only if tech decision changes (with rationale)

## Questions

If uncertain about scope or approach, document the question in your commit message and await human review.
