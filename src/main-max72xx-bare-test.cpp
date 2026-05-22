#include <Arduino.h>
#include <SPI.h>
#include <MD_MAX72xx.h>

#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 4

#define CLK_PIN 5
#define DATA_PIN 18
#define CS_PIN 21

#define PB_SEL_PIN 36
#define PB_DEC_PIN 39
#define PB_INC_PIN 34
#define PB_CNL_PIN 25
#define PB_MEN_PIN 26

MD_MAX72XX matrix(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

struct ButtonProbe {
  const char *name;
  uint8_t pin;
  uint8_t mode;
};

ButtonProbe kButtons[] = {
  {"SEL", PB_SEL_PIN, INPUT},
  {"DEC", PB_DEC_PIN, INPUT},
  {"INC", PB_INC_PIN, INPUT},
  {"CNL", PB_CNL_PIN, INPUT_PULLUP},
  {"MEN", PB_MEN_PIN, INPUT_PULLUP},
};

int s_lastRaw[sizeof(kButtons) / sizeof(kButtons[0])];

void drawPattern(bool invert) {
  uint16_t columns = MAX_DEVICES * 8;
  for (uint16_t c = 0; c < columns; ++c) {
    uint8_t value = (c & 1) ? 0xAA : 0x55;
    if (invert) {
      value = ~value;
    }
    matrix.setColumn(c, value);
  }
}

void drawPerDeviceBar(uint8_t deviceIndex) {
  matrix.clear();
  for (uint8_t row = 0; row < 8; ++row) {
    matrix.setPoint(row, (deviceIndex * 8) + 3, true);
    matrix.setPoint(row, (deviceIndex * 8) + 4, true);
  }
}

void normalizeAllDevices() {
  for (uint8_t dev = 0; dev < MAX_DEVICES; ++dev) {
    matrix.control(dev, MD_MAX72XX::TEST, MD_MAX72XX::OFF);
    matrix.control(dev, MD_MAX72XX::SHUTDOWN, MD_MAX72XX::OFF);
    matrix.control(dev, MD_MAX72XX::INTENSITY, 3);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("[BARE] MAX72xx diagnostic start");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  SPI.begin(CLK_PIN, -1, DATA_PIN, CS_PIN);

  for (size_t i = 0; i < (sizeof(kButtons) / sizeof(kButtons[0])); ++i) {
    pinMode(kButtons[i].pin, kButtons[i].mode);
    s_lastRaw[i] = digitalRead(kButtons[i].pin);
    Serial.print("[BARE] ");
    Serial.print(kButtons[i].name);
    Serial.print(" raw=");
    Serial.println(s_lastRaw[i]);
  }

  if (!matrix.begin()) {
    Serial.println("[BARE] ERROR: matrix.begin() failed");
  }
  normalizeAllDevices();
  matrix.clear();

  Serial.println("[BARE] setup complete");
}

void loop() {
  static uint32_t lastToggle = 0;
  static uint32_t lastDeviceStep = 0;
  static bool invert = false;
  static uint8_t deviceStep = 0;

  if (millis() - lastToggle >= 800) {
    lastToggle = millis();
    normalizeAllDevices();
    drawPattern(invert);
    invert = !invert;

    Serial.print("[BARE] tick ms=");
    Serial.println(lastToggle);
  }

  if (millis() - lastDeviceStep >= 4000) {
    lastDeviceStep = millis();
    normalizeAllDevices();
    drawPerDeviceBar(deviceStep);
    Serial.print("[BARE] per-device bar dev=");
    Serial.println(deviceStep);
    deviceStep = (deviceStep + 1) % MAX_DEVICES;
  }

  for (size_t i = 0; i < (sizeof(kButtons) / sizeof(kButtons[0])); ++i) {
    int raw = digitalRead(kButtons[i].pin);
    if (raw != s_lastRaw[i]) {
      s_lastRaw[i] = raw;
      Serial.print("[BARE][BTN] ");
      Serial.print(kButtons[i].name);
      Serial.print(" raw=");
      Serial.println(raw);
    }
  }
}
