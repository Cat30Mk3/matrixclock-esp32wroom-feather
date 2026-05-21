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

## Phase 2 Kickoff Checklist (Mode and Entry Control)

Implementation checklist:

- Define explicit runtime mode state machine: Normal, AP Setup, Recovery.
- Add mode transition guards to prevent accidental AP entry.
- Implement Menu-hold timing window (4 seconds) with Select confirm window.
- Ensure button handling is ignored for config navigation while in AP mode.
- Implement forced recovery entry on boot combo (Menu + Select held through early boot).
- Ensure Reset-alone path remains a normal reboot with no mode override.
- Add clear serial log markers for each mode transition and trigger source.
- Add display/status indication hooks for AP entry and exit events.

Test checklist (roadmap checkpoint):

- No accidental AP entry during random button use.
- Deterministic AP entry when Menu hold + Select confirm is performed.
- Deterministic recovery AP entry with boot combo + Reset sequence.
- Reset alone does not alter mode unexpectedly.
- Exiting AP mode returns to normal runtime behavior cleanly.

Suggested code touch points:

- src/main.cpp (boot flow + mode bootstrap)
- lib/ISR_Handlers/src/ISR_Handlers.cpp and .h (button event capture)
- lib/Ticker_Manager/src/Ticker_Manager.cpp and .h (timing windows/debounce support)
- New small mode controller module if needed to avoid main.cpp sprawl

## Hardware Validation Snapshot (May 21, 2026)

Post-build upload and runtime smoke test on bench hardware:

- Firmware uploaded successfully.
- Clock setup completed and running correctly.
- WiFi connected on first attempt.
- MQTT messages received and displayed.
- RTC initialized and running.
- No errors observed during this validation run.

## Phase 2 Progress Checkpoint (May 21, 2026)

Implemented in code:

- Added explicit runtime mode state machine with modes: NORMAL, AP_SETUP, RECOVERY.
- Added guarded runtime AP entry sequence: hold Menu for 4s, then Select within 5s confirm window.
- Added boot recovery combo detection window for Menu + Select hold during early boot.
- Added mode transition serial logging and display prompts for AP_SETUP and RECOVERY entry.
- Added mode gate that bypasses normal network startup when AP/Recovery mode is active.
- Updated status adapter to report AP mode activity from runtime mode manager.

Current Phase 2 scope note:

- AP server lifecycle is still Phase 3 work; Phase 2 currently performs deterministic mode entry/control and startup bypass behavior only.

Files added/updated for this checkpoint:

- lib/Mode_Manager/library.json
- lib/Mode_Manager/src/Mode_Manager.h
- lib/Mode_Manager/src/Mode_Manager.cpp
- src/main.cpp
- lib/MatrixClock_Config/src/MatrixClock_Config.cpp
- lib/MatrixClock_Config/library.json
