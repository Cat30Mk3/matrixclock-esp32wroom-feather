# MatrixClock_Config Contract (Phase 0)

MatrixClock_Config is the adapter layer between matrixClock firmware and AP_Config_Portal.

## Responsibilities

- Map matrixClock configuration schema to AP portal fields.
- Register AP pages and fields through AP_Config_Portal.
- Provide load, save, apply, and status callbacks.

## Constraints

- Keep project-specific includes and logic in this library.
- Do not move matrixClock-specific knowledge into AP_Config_Portal.

Phase 0 defines only interfaces and file structure. No setup flow behavior is implemented yet.
