#include <Arduino.h>

struct ButtonDef {
  const char *name;
  uint8_t pin;
  uint8_t mode;
  bool activeLow;
};

// Pin mapping copied from globals.h for isolated hardware validation.
static const ButtonDef kButtons[] = {
  {"SELECT", 36, INPUT, true},
  {"DEC", 39, INPUT, true},
  {"INC", 34, INPUT, true},
  {"CANCEL", 25, INPUT_PULLUP, true},
  {"MENU", 26, INPUT_PULLUP, true}
};

static const uint32_t kDebounceMs = 35;
static const uint32_t kHoldReportStartMs = 1000;
static const uint32_t kHoldReportRepeatMs = 500;

int s_lastRaw[sizeof(kButtons) / sizeof(kButtons[0])];
uint32_t s_lastChangeMs[sizeof(kButtons) / sizeof(kButtons[0])];
uint32_t s_pressStartMs[sizeof(kButtons) / sizeof(kButtons[0])];
uint32_t s_lastHoldReportMs[sizeof(kButtons) / sizeof(kButtons[0])];

bool isPressed(const ButtonDef &button, int raw) {
  return button.activeLow ? (raw == LOW) : (raw == HIGH);
}

void printMapping() {
  Serial.println();
  Serial.println("=== BUTTON TEST: PIN MAPPING ===");
  for (size_t i = 0; i < (sizeof(kButtons) / sizeof(kButtons[0])); ++i) {
    Serial.print("[MAP] ");
    Serial.print(kButtons[i].name);
    Serial.print(" pin=");
    Serial.print(kButtons[i].pin);
    Serial.print(" mode=");
    Serial.print(kButtons[i].mode == INPUT_PULLUP ? "INPUT_PULLUP" : "INPUT");
    Serial.print(" active=");
    Serial.println(kButtons[i].activeLow ? "LOW" : "HIGH");
  }
  Serial.println("================================");
  Serial.println("Press/release each button and watch for matching logs.");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  for (size_t i = 0; i < (sizeof(kButtons) / sizeof(kButtons[0])); ++i) {
    pinMode(kButtons[i].pin, kButtons[i].mode);
    s_lastRaw[i] = digitalRead(kButtons[i].pin);
    s_lastChangeMs[i] = 0;
    s_pressStartMs[i] = 0;
    s_lastHoldReportMs[i] = 0;
  }

  printMapping();
}

void loop() {
  const uint32_t nowMs = millis();

  for (size_t i = 0; i < (sizeof(kButtons) / sizeof(kButtons[0])); ++i) {
    int raw = digitalRead(kButtons[i].pin);
    bool pressed = isPressed(kButtons[i], raw);

    if (raw != s_lastRaw[i]) {
      if ((nowMs - s_lastChangeMs[i]) < kDebounceMs) {
        continue;
      }

      s_lastRaw[i] = raw;
      s_lastChangeMs[i] = nowMs;

      Serial.print("[EDGE] ");
      Serial.print(kButtons[i].name);
      Serial.print(" raw=");
      Serial.print(raw);
      Serial.print(" pressed=");
      Serial.print(pressed ? 1 : 0);

      if (pressed) {
        s_pressStartMs[i] = nowMs;
        s_lastHoldReportMs[i] = nowMs;
        Serial.println(" event=PRESS");
      } else {
        uint32_t heldMs = (s_pressStartMs[i] > 0) ? (nowMs - s_pressStartMs[i]) : 0;
        Serial.print(" event=RELEASE heldMs=");
        Serial.println(heldMs);
        s_pressStartMs[i] = 0;
        s_lastHoldReportMs[i] = 0;
      }
    }

    if (pressed && s_pressStartMs[i] > 0) {
      uint32_t heldMs = nowMs - s_pressStartMs[i];
      if (heldMs >= kHoldReportStartMs && (nowMs - s_lastHoldReportMs[i]) >= kHoldReportRepeatMs) {
        s_lastHoldReportMs[i] = nowMs;
        Serial.print("[HOLD] ");
        Serial.print(kButtons[i].name);
        Serial.print(" heldMs=");
        Serial.println(heldMs);
      }
    }
  }

  delay(5);
}
