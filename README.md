
# matrixClock - PlatformIO Migration

This is a PlatformIO-compatible version of the matrixClock project, migrated from Arduino IDE.
<p align="center">
<img width="45%"  alt="IMG_0107" src="https://github.com/user-attachments/assets/5e8846ef-ca00-469f-88d9-23f22f367ac9" />
<img width="45%"  alt="IMG_0108" src="https://github.com/user-attachments/assets/b1306a2d-ab0d-4750-bf5c-1b8e41943e24" />
</p>  
<p align="center">
<img width="45%"  alt="IMG_0113" src="https://github.com/user-attachments/assets/e87198d6-6bbf-4e5a-8db9-b69ce222cbf4" />
<img width="45%"  alt="IMG_0113" src="https://github.com/user-attachments/assets/6fabb513-ec90-4e7d-99e3-d90884cb1552" />
</p>



## Features
- **LED Matrix Display**: Supports multiple display configurations with vertical and horizontal scrolling
- **Temperature Monitoring**: Local DS18B20 sensors and remote MQTT temperature data
- **WiFi & MQTT**: Full MQTT integration for remote sensor data and control
- **Time Management**: RTC with timezone support and NTP synchronization
- **Non-Blocking Operations**: All time-sensitive operations use Ticker for non-blocking execution
- **ISR Handlers**: Button inputs for user interaction

## Project Structure

```
├── src/
│   └── main.cpp                 # Main application entry point (setup & loop)
├── include/
│   ├── Font_Data.h              # LED display font definitions
│   ├── secrets_template.h       # Template for local secrets configuration
│   └── secrets.h                # Local secrets (gitignored)
├── lib/
│   ├── globals/                 # Shared globals, configs, and data structures
│   ├── Parola_Display/          # LED display management (MD_Parola wrapper)
│   ├── WiFi_Manager/            # WiFi connectivity
│   ├── MQTT_Manager/            # MQTT protocol handling
│   ├── JSON_Handler/            # JSON parsing for MQTT payloads
│   ├── Time_Manager/            # Time management, RTC, and timezone handling
│   ├── DS18B20_Manager/         # Temperature sensor management
│   ├── NTP_Manager/             # NTP time synchronization
│   ├── ISR_Handlers/            # Button interrupt handlers
│   └── Ticker_Manager/          # Non-blocking timers and ISRs
└── platformio.ini               # PlatformIO configuration

```


## Dependencies

All dependencies are managed by PlatformIO and defined in `platformio.ini`:

- **MD_MAX72XX** - LED matrix driver
- **MD_Parola** - LED display animations and text effects
- **PubSubClient** - MQTT client
- **ArduinoJson** - JSON parsing (v7.1.0)
- **Timezone** - Timezone management
- **Time** - Time library
- **RTClib** - RTC (DS3231) driver
- **OneWire** - OneWire protocol
- **DallasTemperature** - DS18B20 temperature sensors
- **Ticker** - Non-blocking timers

## Configuration

### Display Configuration
Edit `lib/globals/src/globals.h`:
```cpp
#define DISPLAY_CONFIG DISPLAY2X8  // or DISPLAY1X4
```

### Temperature Sensors
Configure in `lib/globals/src/globals.h`:
```cpp
#define SWAP_DS18B20 1              // Swap sensor indices
#define ONE_TEMP_IS_IN 1            // Configure indoor/outdoor
```

### Hardware Pins
All GPIO pins are defined in `lib/globals/src/globals.h`.

## <u>Secrets WiFi  and MQTT Credentials</u>
This project supports a public-safe secrets pattern:

- `include/secrets_template.h` is committed and contains dummy values.
- `include/secrets.h` contains real credentials and is gitignored.

Build behavior:

- If `include/secrets.h` exists, it is used.
- If missing, `include/secrets_template.h` is used and compile-time warning is emitted.

First-time local setup

- Copy `include/secrets_template.h` to `include/secrets.h`.
- Replace WiFi and MQTT entries with your real credentials and pub/sub topics.
- note SMTP has not been implemented
- Keep `include/secrets.h` local only (already ignored by `.gitignore`).

## Building & Uploading

### Build:
```bash
platformio run -e esp32doit-devkit-v1
```

### Upload:
```bash
platformio run -e esp32doit-devkit-v1 -t upload
```

### Monitor:
```bash
platformio run -e esp32doit-devkit-v1 -t monitor
```

## Library Architecture

### globals
Central repository for all global variables, structures, and configuration. All other libraries depend on this.

### Parola_Display
Handles LED matrix display operations including text scrolling, zones, and animations. Wraps MD_Parola library.

### WiFi_Manager
Manages WiFi connection/reconnection logic with visual LED feedback.

### MQTT_Manager
Handles MQTT connections, subscriptions, and message publishing. Includes callback for received messages.

### JSON_Handler
Parses JSON payloads from MQTT messages and populates display parameters.

### Time_Manager
Manages time from RTC (DS3231), timezone conversion, and time formatting functions.

### DS18B20_Manager
Handles OneWire DS18B20 temperature sensor enumeration and readings.

### NTP_Manager
Handles NTP synchronization for accurate time keeping.

### ISR_Handlers
Button interrupt service routines for push buttons.

### Ticker_Manager
Non-blocking timing functions and ISRs that avoid blocking the main loop:
- Temperature sensor read cycles
- MQTT keep-alive
- LED blinking
- Non-blocking delays

## Version

Migrated from: matrixClock_consolidated_ver_1_1_0  
PlatformIO Version: 1.0.0  
Platform: ESP32 (esp32doit-devkit-v1)  
Framework: Arduino

## Author

Original: John Witherspoon  
Last Updated: May 4, 2026
