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
