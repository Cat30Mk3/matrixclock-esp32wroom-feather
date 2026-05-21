# Branch Notes: feature/web-config-ap-portal

## Verified Stability Fixes (May 20, 2026)

Hardware test result on DISPLAY1X4: 30-minute run with no all-pixels-lit display failure.

Most likely root cause was concurrent access to the Parola display object from ticker callback context and main loop context.

Applied fix commit:

- 2e6991d fix: remove concurrent Parola calls from ticker callbacks

Related robustness fix committed during the same validation cycle:

- 52227ca fix: guard RTC flow when rtc.begin fails

## Main Branch Follow-Up

When this work is promoted, cherry-pick at minimum:

- 2e6991d

Recommended also:

- 52227ca

## Test Configuration Snapshot

Current branch test configuration includes:

- DISPLAY_CONFIG set to DISPLAY1X4 in lib/globals/src/globals.h
- platformio.ini upload/monitor port set to COM8 for local bench hardware

## Phase 1 Completion Checkpoint (May 21, 2026)

Phase 1 objectives from product spec roadmap are implemented and validated:

- Runtime config persistence and schema versioning are implemented in MatrixClock_Config.
- NVS validity checks are enforced (schema match + full blob length) before load.
- Fallback chain behavior is in place: valid NVS first, otherwise bootstrap defaults from secrets source.
- First-load bootstrap seeding writes runtime config to NVS after fallback.
- AP portal field schema registration now performs duplicate and structural validation.
- Adapter field-to-config mapping self-check runs during setup and logs pass/fail.

Validation evidence:

- Build command: python -m platformio run -e featheresp32
- Result: SUCCESS (firmware linked and binary generated)

Files touched for this checkpoint:

- lib/AP_Config_Portal/src/AP_Config_Portal.cpp
- lib/MatrixClock_Config/src/MatrixClock_Config.h
- lib/MatrixClock_Config/src/MatrixClock_Config.cpp
- src/main.cpp
