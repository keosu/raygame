# Working in this repository

- Read `docs/AGENT_STATE.md` before continuing work, then the relevant part of
  `docs/PRODUCT_READINESS_REVIEW.md`. Treat the state file as a handoff, and verify
  the working tree and current code before relying on it.
- Keep `docs/AGENT_STATE.md` current after each substantial work package and before
  handing work back: record completed changes, validation, known limitations and
  concrete next steps. Clearly separate passing checks from checks not yet run.
- The user prefers Chinese progress reports, continued implementation without
  repeated permission requests, and commits for completed, verified work packages.
- Windows/MSVC through xmake is the primary local validation path. Keep the four
  examples, logic tests and graphics smoke target working; preserve Linux builds.
- Example games launch without a console, retain file diagnostics, follow the
  native Windows theme, and support aspect-preserving adaptive layouts.
- Use PowerShell without a profile for shell commands. Preserve UTF-8/LF source
  files. Do not overwrite `build_err.txt` with new logs; use ignored `build/` paths.
- Do not create sub-agents unless the user explicitly requests delegation.
