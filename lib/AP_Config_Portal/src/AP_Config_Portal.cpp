#include "AP_Config_Portal.h"
#include <WebServer.h>

namespace {
static const size_t kMaxPages = 8;
static const size_t kMaxFields = 64;
static const uint16_t kDefaultHttpPort = 80;
static const size_t kFieldValueBufferLen = 128;

struct RegisteredPage {
  const char *pageId;
  const char *title;
};

struct RegisteredField {
  APFieldDefinition field;
};

static RegisteredPage s_pages[kMaxPages];
static size_t s_pageCount = 0;
static RegisteredField s_fields[kMaxFields];
static size_t s_fieldCount = 0;
static WebServer *s_server = nullptr;
static uint16_t s_serverPort = kDefaultHttpPort;
static bool s_serverRunning = false;
static bool s_authenticated = false;
static char s_loginPin[5] = "0000";
static APPortalCallbacks s_callbacks = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

bool hasText(const char *value) {
  return value != nullptr && value[0] != '\0';
}

bool pageExists(const char *pageId) {
  for (size_t i = 0; i < s_pageCount; ++i) {
    if (strcmp(s_pages[i].pageId, pageId) == 0) {
      return true;
    }
  }
  return false;
}

bool fieldExists(const char *fieldId) {
  for (size_t i = 0; i < s_fieldCount; ++i) {
    if (strcmp(s_fields[i].field.fieldId, fieldId) == 0) {
      return true;
    }
  }
  return false;
}

String boolText(bool value) {
  return value ? "YES" : "NO";
}

String htmlEscape(const String &input) {
  String output;
  output.reserve(input.length() + 16);

  for (size_t i = 0; i < input.length(); ++i) {
    char c = input[i];
    if (c == '&') output += "&amp;";
    else if (c == '<') output += "&lt;";
    else if (c == '>') output += "&gt;";
    else if (c == '"') output += "&quot;";
    else output += c;
  }

  return output;
}

String getFieldValueOrEmpty(const APFieldDefinition &field);
String renderInputForField(const APFieldDefinition &field, const String &value);
void renderLoginPage(const String &message);
void renderPortalPage(const String &message);
bool ensureAuthenticated();

void generateLoginPin() {
  uint32_t value = esp_random() % 9000UL;
  value += 1000UL;
  snprintf(s_loginPin, sizeof(s_loginPin), "%04lu", static_cast<unsigned long>(value));
}

void sendRedirect(const char *path) {
  if (s_server == nullptr) {
    return;
  }

  s_server->sendHeader("Location", path);
  s_server->send(302, "text/plain", "");
}

void handleRoot() {
  if (s_authenticated) {
    sendRedirect("/portal");
    return;
  }

  renderLoginPage("");
}

void handleLogin() {
  if (s_server == nullptr) {
    return;
  }

  if (!s_server->hasArg("pin")) {
    renderLoginPage("PIN required");
    return;
  }

  const String enteredPin = s_server->arg("pin");
  if (enteredPin == String(s_loginPin)) {
    s_authenticated = true;
    if (s_callbacks.loadConfig != nullptr) {
      s_callbacks.loadConfig(s_callbacks.context);
    }
    sendRedirect("/portal");
    return;
  }

  renderLoginPage("Invalid PIN");
}

void handlePortal() {
  if (!ensureAuthenticated()) {
    return;
  }

  renderPortalPage("");
}

void handleSave() {
  if (!ensureAuthenticated()) {
    return;
  }

  bool ok = true;
  for (size_t i = 0; i < s_fieldCount; ++i) {
    const APFieldDefinition &field = s_fields[i].field;
    String value;

    if (field.type == AP_FIELD_TOGGLE) {
      value = s_server->hasArg(field.fieldId) ? "1" : "0";
    } else {
      if (!s_server->hasArg(field.fieldId)) {
        continue;
      }
      value = s_server->arg(field.fieldId);
    }

    if (s_callbacks.setFieldValue != nullptr) {
      ok = s_callbacks.setFieldValue(s_callbacks.context, field.fieldId, value.c_str()) && ok;
    }
  }

  if (ok && s_callbacks.saveConfig != nullptr) {
    ok = s_callbacks.saveConfig(s_callbacks.context) && ok;
  }

  if (ok && s_callbacks.applyConfig != nullptr) {
    ok = s_callbacks.applyConfig(s_callbacks.context) && ok;
  }

  renderPortalPage(ok ? "Settings saved" : "Save failed");
}

void handleLogout() {
  s_authenticated = false;
  sendRedirect("/");
}

bool ensureAuthenticated() {
  if (s_authenticated) {
    return true;
  }

  sendRedirect("/");
  return false;
}

String getFieldValueOrEmpty(const APFieldDefinition &field) {
  if (s_callbacks.getFieldValue == nullptr) {
    return "";
  }

  char valueBuffer[kFieldValueBufferLen] = {0};
  if (!s_callbacks.getFieldValue(s_callbacks.context, field.fieldId, valueBuffer, sizeof(valueBuffer))) {
    return "";
  }

  return String(valueBuffer);
}

String renderInputForField(const APFieldDefinition &field, const String &value) {
  String html;
  const String escapedFieldId = htmlEscape(String(field.fieldId));
  const String escapedValue = htmlEscape(value);

  html += "<label for='" + escapedFieldId + "'>" + htmlEscape(String(field.label)) + "</label>";

  if (field.type == AP_FIELD_TOGGLE) {
    const bool enabled = (value == "1" || value.equalsIgnoreCase("true") || value.equalsIgnoreCase("on"));
    html += "<input type='checkbox' id='" + escapedFieldId + "' name='" + escapedFieldId + "' value='1'";
    if (enabled) {
      html += " checked";
    }
    html += ">";
    return html;
  }

  const char *inputType = "text";
  if (field.type == AP_FIELD_PASSWORD) {
    inputType = "password";
  } else if (field.type == AP_FIELD_NUMBER) {
    inputType = "number";
  }

  html += "<input type='";
  html += inputType;
  html += "' id='" + escapedFieldId + "' name='" + escapedFieldId + "' maxlength='";
  html += String(field.maxLength);
  html += "' value='" + escapedValue + "'>";
  return html;
}

void renderLoginPage(const String &message) {
  if (s_server == nullptr) {
    return;
  }

  String html;
  html.reserve(1024);
  html += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>matrixClock AP Login</title></head><body>";
  html += "<h1>matrixClock AP Setup</h1>";
  html += "<p>Connect to AP SSID and enter the one-time PIN.</p>";
  if (message.length() > 0) {
    html += "<p><strong>" + htmlEscape(message) + "</strong></p>";
  }
  html += "<form method='POST' action='/login'>";
  html += "<label for='pin'>PIN</label>";
  html += "<input type='password' id='pin' name='pin' maxlength='4' inputmode='numeric' required>";
  html += "<button type='submit'>Login</button></form>";
  html += "</body></html>";

  s_server->send(200, "text/html", html);
}

void renderPortalPage(const String &message) {
  if (s_server == nullptr) {
    return;
  }

  APPortalStatus status = {false, false, false, false, "UNKNOWN"};
  if (s_callbacks.getStatus != nullptr) {
    s_callbacks.getStatus(s_callbacks.context, status);
  }

  String html;
  html.reserve(8192);
  html += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>matrixClock Portal</title></head><body>";
  html += "<h1>matrixClock Portal</h1>";
  html += "<p><a href='/logout'>Logout</a></p>";
  if (message.length() > 0) {
    html += "<p><strong>" + htmlEscape(message) + "</strong></p>";
  }

  html += "<h2>Status</h2><ul>";
  html += "<li>AP Mode Active: " + boolText(status.apModeActive) + "</li>";
  html += "<li>Station Connected: " + boolText(status.stationConnected) + "</li>";
  html += "<li>MQTT Connected: " + boolText(status.mqttConnected) + "</li>";
  html += "<li>Time Valid: " + boolText(status.timeValid) + "</li>";
  html += "<li>Time Source: " + htmlEscape(String(status.timeSourceMode == nullptr ? "UNKNOWN" : status.timeSourceMode)) + "</li>";
  html += "</ul>";

  html += "<form method='POST' action='/save'>";
  for (size_t pageIndex = 0; pageIndex < s_pageCount; ++pageIndex) {
    const char *pageId = s_pages[pageIndex].pageId;
    html += "<h2>" + htmlEscape(String(s_pages[pageIndex].title)) + "</h2>";

    for (size_t fieldIndex = 0; fieldIndex < s_fieldCount; ++fieldIndex) {
      const APFieldDefinition &field = s_fields[fieldIndex].field;
      if (strcmp(field.pageId, pageId) != 0) {
        continue;
      }

      const String fieldValue = getFieldValueOrEmpty(field);
      html += "<div style='margin:0 0 10px 0'>";
      html += renderInputForField(field, fieldValue);
      html += "</div>";
    }
  }

  html += "<button type='submit'>Save & Apply</button></form>";
  html += "</body></html>";

  s_server->send(200, "text/html", html);
}
}

void apPortalBegin() {
  s_pageCount = 0;
  s_fieldCount = 0;
}

void apPortalEnd() {
  apPortalStopServer();
}

bool apPortalStartServer(uint16_t port) {
  if (s_serverRunning) {
    return true;
  }

  s_serverPort = (port == 0) ? kDefaultHttpPort : port;
  s_server = new WebServer(s_serverPort);
  if (s_server == nullptr) {
    return false;
  }

  generateLoginPin();
  s_authenticated = false;

  s_server->on("/", HTTP_GET, handleRoot);
  s_server->on("/login", HTTP_POST, handleLogin);
  s_server->on("/portal", HTTP_GET, handlePortal);
  s_server->on("/save", HTTP_POST, handleSave);
  s_server->on("/logout", HTTP_GET, handleLogout);

  s_server->begin();
  s_serverRunning = true;
  return true;
}

void apPortalStopServer() {
  if (s_server != nullptr) {
    s_server->stop();
    delete s_server;
    s_server = nullptr;
  }

  s_serverRunning = false;
  s_authenticated = false;
}

void apPortalService() {
  if (!s_serverRunning || s_server == nullptr) {
    return;
  }

  s_server->handleClient();
}

bool apPortalIsServerRunning() {
  return s_serverRunning;
}

const char *apPortalGetLoginPin() {
  return s_loginPin;
}

bool apPortalRegisterPage(const char *pageId, const char *title) {
  if (!hasText(pageId) || !hasText(title)) {
    return false;
  }

  if (s_pageCount >= kMaxPages) {
    return false;
  }

  if (pageExists(pageId)) {
    return false;
  }

  s_pages[s_pageCount].pageId = pageId;
  s_pages[s_pageCount].title = title;
  ++s_pageCount;

  return true;
}

bool apPortalRegisterField(const APFieldDefinition &field) {
  if (!hasText(field.pageId) || !hasText(field.fieldId) || !hasText(field.label)) {
    return false;
  }

  if (!pageExists(field.pageId)) {
    return false;
  }

  if (field.maxLength == 0) {
    return false;
  }

  if (field.optionCount > 0 && field.options == nullptr) {
    return false;
  }

  if (s_fieldCount >= kMaxFields) {
    return false;
  }

  if (fieldExists(field.fieldId)) {
    return false;
  }

  s_fields[s_fieldCount].field = field;
  ++s_fieldCount;

  return true;
}

void apPortalSetCallbacks(const APPortalCallbacks &callbacks) {
  s_callbacks = callbacks;
}
