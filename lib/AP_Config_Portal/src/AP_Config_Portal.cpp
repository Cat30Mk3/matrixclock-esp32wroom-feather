#include "AP_Config_Portal.h"
#include <WebServer.h>
#include <DNSServer.h>

namespace {

// ─── Constants ────────────────────────────────────────────────────────────────
static const size_t   kMaxPages         = 16;
static const size_t   kMaxFields        = 64;
static const uint16_t kDefaultHttpPort  = 80;
static const uint8_t  kDnsPort          = 53;
static const size_t   kFieldValueBufLen = 128;
static const size_t   kSessionTokenLen  = 16;
static const uint32_t kInactivityMs     = 300000UL; // 5 minutes
static const uint32_t kPostExitDelayMs  =   2000UL; // delay after exit page is served

// ─── Data structures ──────────────────────────────────────────────────────────
struct RegisteredPage  { const char *pageId; const char *title; };
struct RegisteredField { APFieldDefinition field; };

// ─── State ────────────────────────────────────────────────────────────────────
static RegisteredPage  s_pages[kMaxPages];
static size_t          s_pageCount      = 0;
static RegisteredField s_fields[kMaxFields];
static size_t          s_fieldCount     = 0;

static WebServer  *s_server       = nullptr;
static DNSServer  *s_dns          = nullptr;
static uint16_t    s_serverPort   = kDefaultHttpPort;
static bool        s_serverRunning= false;

static char     s_loginPin[5]                     = "0000";
static bool     s_authenticated                    = false;
static uint32_t s_lastActivityMs                   = 0;
static bool     s_hasUnsavedChanges                = false;

static APDisplayStage s_displayStage    = AP_STAGE_SSID;
static bool           s_exitRequested   = false;
static bool           s_exitWasSave     = false;
static uint32_t       s_exitScheduledMs = 0;

static char s_flashMessage[80] = {0};

static APPortalCallbacks s_callbacks = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

// ─── Forward declarations ─────────────────────────────────────────────────────
void renderLoginPage(const String &message);
void renderMenuPage();
void renderInfoPage();
void renderConfigPage(const char *pageId);
void handleSaveConfigPage(const char *pageId);
void renderExitPage(bool saved);
void recordActivity();
String consumeFlash();

// ─── Utilities ────────────────────────────────────────────────────────────────
bool hasText(const char *v) { return v != nullptr && v[0] != '\0'; }

bool pageExists(const char *pageId) {
  for (size_t i = 0; i < s_pageCount; ++i)
    if (strcmp(s_pages[i].pageId, pageId) == 0) return true;
  return false;
}

bool fieldExists(const char *fieldId) {
  for (size_t i = 0; i < s_fieldCount; ++i)
    if (strcmp(s_fields[i].field.fieldId, fieldId) == 0) return true;
  return false;
}

String htmlEscape(const String &s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    if      (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else               out += c;
  }
  return out;
}

void sendRedirect(const char *path) {
  if (!s_server) return;
  s_server->sendHeader("Connection", "close");
  s_server->sendHeader("Location", path);
  s_server->send(302, "text/plain", "");
}

// ─── PIN / session ────────────────────────────────────────────────────────────
void generateLoginPin() {
  uint32_t v = (esp_random() % 9000UL) + 1000UL;
  snprintf(s_loginPin, sizeof(s_loginPin), "%04lu", (unsigned long)v);
}

// Per spec: an invalid/expired session closes the AP entirely, does not loop back to PIN.
// A simple boolean is used instead of a cookie token — the iOS CNA captive portal
// webview does not reliably forward cookies across page navigations.
bool ensureAuthenticated() {
  if (s_authenticated) return true;
  s_exitRequested = true;
  s_exitWasSave   = false;
  sendRedirect("/");
  return false;
}

// ─── Activity / timeout / flash ───────────────────────────────────────────────
void recordActivity() { s_lastActivityMs = millis(); }

void setFlash(const char *msg) { strlcpy(s_flashMessage, msg, sizeof(s_flashMessage)); }

String consumeFlash() {
  if (s_flashMessage[0] == '\0') return "";
  String m = String(s_flashMessage);
  s_flashMessage[0] = '\0';
  return m;
}

// ─── Field value helper ───────────────────────────────────────────────────────
String getFieldValue(const APFieldDefinition &field) {
  if (!s_callbacks.getFieldValue) return "";
  char buf[kFieldValueBufLen] = {0};
  if (!s_callbacks.getFieldValue(s_callbacks.context, field.fieldId, buf, sizeof(buf))) return "";
  return String(buf);
}

// ─── Shared CSS (Tasmota-inspired) ────────────────────────────────────────────
// Stored as a raw string literal to keep it readable and avoid escape issues.
static const char kPageCss[] =
  "body{font-family:sans-serif;background:#1a1a1a;margin:0;padding:10px;}"
  ".c{max-width:500px;margin:0 auto;background:#fff;border-radius:6px;overflow:hidden;}"
  "h1{background:#1fa3ec;color:#fff;padding:8px 14px;margin:0;font-size:1.1em;font-weight:bold;}"
  "h2{color:#333;margin:14px 14px 4px;font-size:.95em;border-bottom:1px solid #ddd;padding-bottom:4px;}"
  ".bdy{padding:10px 14px;}"
  "fieldset{border:1px solid #ccc;border-radius:4px;padding:10px;margin:8px 0;}"
  "legend{font-weight:bold;color:#555;padding:0 6px;font-size:.9em;}"
  "label{display:block;font-size:.85em;color:#555;margin-bottom:2px;}"
  "input[type=text],input[type=password],input[type=number]"
  "{width:100%;box-sizing:border-box;padding:8px;border:1px solid #ccc;"
  "border-radius:4px;font-size:1em;margin-bottom:8px;}"
  ".btn{display:block;width:100%;padding:11px;border:0;border-radius:4px;"
  "font-size:1em;font-family:inherit;cursor:pointer;margin:5px 0;color:#fff;text-align:center;"
  "text-decoration:none;box-sizing:border-box;}"
  ".bn{background:#1fa3ec;}.bg{background:#4caf50;}"
  ".br{background:#e53935;}.bk{background:#888;}"
  ".msg{background:#fff3cd;border:1px solid #ffc107;padding:8px;border-radius:4px;"
  "margin:6px 0;color:#555;font-size:.9em;}"
  ".ok{background:#d4edda;border-color:#28a745;color:#155724;}"
  "table{width:100%;border-collapse:collapse;font-size:.9em;}"
  "td{padding:5px 4px;border-bottom:1px solid #eee;}"
  "td:first-child{font-weight:bold;color:#555;width:44%;}"
  ".tgl{display:flex;align-items:center;margin-bottom:8px;}"
  ".tgl input{width:auto;margin:0 8px 0 0;}"
  ".tgl label{margin:0;font-size:1em;color:#333;}";

// ─── Page head / foot ─────────────────────────────────────────────────────────
String pageHead(const char *title) {
  String h;
  h.reserve(600);
  h += F("<!doctype html><html><head>"
         "<meta charset='utf-8'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>");
  h += htmlEscape(String(title));
  h += F("</title><style>");
  h += kPageCss;
  h += F("</style></head><body><div class='c'><h1>");
  h += htmlEscape(String(title));
  h += F("</h1><div class='bdy'>");
  return h;
}

String pageFoot() { return F("</div></div></body></html>"); }

// ─── Field renderer ───────────────────────────────────────────────────────────
String renderField(const APFieldDefinition &f, const String &value) {
  String html;
  const String eid = htmlEscape(String(f.fieldId));
  const String lbl = htmlEscape(String(f.label));
  const String esc = htmlEscape(value);

  if (f.type == AP_FIELD_TOGGLE) {
    bool en = (value == "1" || value.equalsIgnoreCase("true") || value.equalsIgnoreCase("on"));
    html += F("<div class='tgl'><input type='checkbox' id='");
    html += eid; html += F("' name='"); html += eid; html += F("' value='1'");
    if (en) html += F(" checked");
    html += F("><label for='"); html += eid; html += F("'>"); html += lbl;
    html += F("</label></div>");
    return html;
  }

  html += F("<label for='"); html += eid; html += F("'>"); html += lbl; html += F("</label>");
  const char *itype = (f.type == AP_FIELD_PASSWORD) ? "password" :
                      (f.type == AP_FIELD_NUMBER)   ? "number"   : "text";
  html += F("<input type='"); html += itype;
  html += F("' id='"); html += eid;
  html += F("' name='"); html += eid;
  html += F("' maxlength='"); html += String(f.maxLength);
  html += F("' value='"); html += esc; html += F("'>");
  return html;
}

// ─── Page renderers ───────────────────────────────────────────────────────────

void renderLoginPage(const String &message) {
  if (!s_server) return;
  String html = pageHead("matrixClock AP Setup");
  html += F("<p style='color:#555;font-size:.9em;'>"
            "Look at the clock display for the 4-digit PIN.</p>");
  if (message.length())
    html += "<div class='msg'>" + htmlEscape(message) + "</div>";
  html += F("<form method='POST' action='/login'>"
            "<label for='pin'>PIN</label>"
            "<input type='password' id='pin' name='pin' maxlength='4'"
            " inputmode='numeric' autocomplete='off' required>"
            "<button type='submit' class='btn bn'>Login</button>"
            "</form>");
  html += pageFoot();
  s_server->sendHeader("Connection", "close");
  s_server->send(200, "text/html", html);
}

void renderMenuPage() {
  if (!s_server) return;
  String flash = consumeFlash();

  APPortalStatus st = {};
  if (s_callbacks.getStatus) s_callbacks.getStatus(s_callbacks.context, st);

  String html = pageHead("matrixClock Config");
  if (flash.length())
    html += "<div class='msg ok'>" + htmlEscape(flash) + "</div>";
  if (s_hasUnsavedChanges)
    html += F("<div class='msg'>Unsaved changes &mdash; use <b>Save &amp; Exit</b> to keep them.</div>");

  html += F("<table>");
  html += "<tr><td>WiFi STA</td><td>" + String(st.stationConnected ? "Connected" : "Not connected") + "</td></tr>";
  html += "<tr><td>MQTT</td><td>" + String(st.mqttConnected ? "Connected" : "Not connected") + "</td></tr>";
  html += "<tr><td>Time</td><td>" + String(st.timeValid ? "Valid" : "Invalid") + "</td></tr>";
  if (st.uptimeSeconds > 0)
    html += "<tr><td>Uptime</td><td>" + String(st.uptimeSeconds) + "s</td></tr>";
  html += F("</table>");

  html += F("<h2>Configuration</h2>");
  html += F("<button type='button' class='btn bn' onclick='location.href=\"/info\"'>Device Info</button>");
  for (size_t i = 0; i < s_pageCount; ++i) {
    html += "<button type='button' class='btn bn' onclick='location.href=\"/" +
            String(s_pages[i].pageId) + "\"'>" +
            htmlEscape(String(s_pages[i].title)) + "</button>";
  }

  html += F("<h2>Session</h2>");
  html += F("<button type='button' class='btn bg' onclick='location.href=\"/exit/save\"'>Save &amp; Exit</button>");
  html += F("<button type='button' class='btn bk' onclick='location.href=\"/exit/cancel\"'>Cancel &amp; Exit</button>");

  html += pageFoot();
  s_server->sendHeader("Connection", "close");
  s_server->send(200, "text/html", html);
}

void renderInfoPage() {
  if (!s_server) return;
  APPortalStatus st = {};
  if (s_callbacks.getStatus) s_callbacks.getStatus(s_callbacks.context, st);

  String html = pageHead("Device Info");
  html.reserve(2048);
  html += F("<table>");
  if (hasText(st.projectName))
    html += "<tr><td>Project</td><td>" + htmlEscape(String(st.projectName)) + "</td></tr>";
  if (hasText(st.firmwareVersion))
    html += "<tr><td>Version</td><td>" + htmlEscape(String(st.firmwareVersion)) + "</td></tr>";
  if (hasText(st.compileDate))
    html += "<tr><td>Compiled</td><td>" + htmlEscape(String(st.compileDate)) + " " +
            htmlEscape(String(st.compileTime ? st.compileTime : "")) + "</td></tr>";
  if (st.uptimeSeconds > 0)
    html += "<tr><td>Uptime</td><td>" + String(st.uptimeSeconds) + "s</td></tr>";
  html += "<tr><td>AP SSID</td><td>" + htmlEscape(String(st.apSsid ? st.apSsid : "")) + "</td></tr>";
  html += "<tr><td>AP IP</td><td>" + htmlEscape(String(st.apIp ? st.apIp : "192.168.4.1")) + "</td></tr>";
  html += "<tr><td>WiFi STA</td><td>" + String(st.stationConnected ? "Connected" : "Not connected") + "</td></tr>";
  html += "<tr><td>MQTT</td><td>" + String(st.mqttConnected ? "Connected" : "Not connected") + "</td></tr>";
  html += "<tr><td>Time source</td><td>" + htmlEscape(String(st.timeValid ? (st.timeSourceMode ? st.timeSourceMode : "OK") : "Invalid")) + "</td></tr>";
  html += F("</table>");
  html += F("<button type='button' class='btn bn' style='margin-top:12px' onclick='location.href=\"/menu\"'>&#8592; Main Menu</button>");
  html += pageFoot();
  s_server->sendHeader("Connection", "close");
  s_server->send(200, "text/html", html);
}

void renderConfigPage(const char *pageId) {
  if (!s_server) return;
  const char *pageTitle = pageId;
  for (size_t i = 0; i < s_pageCount; ++i)
    if (strcmp(s_pages[i].pageId, pageId) == 0) { pageTitle = s_pages[i].title; break; }

  String flash = consumeFlash();
  String html = pageHead(pageTitle);
  if (flash.length())
    html += "<div class='msg ok'>" + htmlEscape(flash) + "</div>";

  html += "<form method='POST' action='/" + String(pageId) + "'>";
  html += F("<fieldset><legend>Settings</legend>");

  bool anyField = false;
  for (size_t i = 0; i < s_fieldCount; ++i) {
    const APFieldDefinition &f = s_fields[i].field;
    if (strcmp(f.pageId, pageId) != 0) continue;
    anyField = true;
    html += renderField(f, getFieldValue(f));
  }
  if (!anyField)
    html += F("<p style='color:#888;'>No fields registered for this page.</p>");

  html += F("</fieldset>");
  html += F("<button type='submit' class='btn bg'>Apply</button>");
  html += F("</form>");
  html += F("<button type='button' class='btn bn' onclick='location.href=\"/menu\"'>&#8592; Main Menu</button>");
  html += pageFoot();
  s_server->sendHeader("Connection", "close");
  s_server->send(200, "text/html", html);
}

void handleSaveConfigPage(const char *pageId) {
  if (!s_server) return;
  for (size_t i = 0; i < s_fieldCount; ++i) {
    const APFieldDefinition &f = s_fields[i].field;
    if (strcmp(f.pageId, pageId) != 0) continue;
    String value;
    if (f.type == AP_FIELD_TOGGLE) {
      value = s_server->hasArg(f.fieldId) ? "1" : "0";
    } else {
      if (!s_server->hasArg(f.fieldId)) continue;
      value = s_server->arg(f.fieldId);
    }
    if (s_callbacks.setFieldValue)
      s_callbacks.setFieldValue(s_callbacks.context, f.fieldId, value.c_str());
  }
  // Apply to live RAM only — NVS write happens at Save & Exit
  if (s_callbacks.applyConfig)
    s_callbacks.applyConfig(s_callbacks.context);

  s_hasUnsavedChanges = true;

  const char *pageTitle = pageId;
  for (size_t i = 0; i < s_pageCount; ++i)
    if (strcmp(s_pages[i].pageId, pageId) == 0) { pageTitle = s_pages[i].title; break; }

  char msg[80];
  snprintf(msg, sizeof(msg), "%s applied \xe2\x80\x94 use Save & Exit to persist.", pageTitle);
  setFlash(msg);
  sendRedirect("/menu");
}

void renderExitPage(bool saved) {
  if (!s_server) return;
  String html = pageHead("matrixClock AP Setup");
  if (saved)
    html += F("<div class='msg ok'><b>Settings saved.</b> Returning to clock mode&hellip;</div>");
  else
    html += F("<div class='msg'>Changes discarded. Returning to clock mode&hellip;</div>");
  html += F("<p style='color:#888;font-size:.9em;'>The AP will close in a moment.</p>");
  html += pageFoot();
  s_server->sendHeader("Connection", "close");
  s_server->send(200, "text/html", html);
}

// ─── Route handlers ───────────────────────────────────────────────────────────

void handleRoot() {
  recordActivity();
  if (s_authenticated) { sendRedirect("/menu"); return; }
  // Advance display stage from SSID/IP to PIN when user opens the portal
  if (s_displayStage < AP_STAGE_PIN) s_displayStage = AP_STAGE_PIN;
  renderLoginPage("");
}

void handleLogin() {
  recordActivity();
  if (!s_server) return;
  if (!s_server->hasArg("pin")) { renderLoginPage("PIN required"); return; }
  const String entered = s_server->arg("pin");
  if (entered != String(s_loginPin)) { renderLoginPage("Invalid PIN \xe2\x80\x94 check display"); return; }

  s_authenticated = true;
  s_displayStage  = AP_STAGE_ACTIVE;
  if (s_callbacks.loadConfig) s_callbacks.loadConfig(s_callbacks.context);
  s_hasUnsavedChanges = false;

  sendRedirect("/menu");
}

void handleMenu() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  renderMenuPage();
}

void handleInfo() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  renderInfoPage();
}

// Shared GET handler for all registered config pages (/wifi, /mqtt, …)
void handleConfigPageGet() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  if (!s_server) return;
  String pageId = s_server->uri().substring(1); // strip leading '/'
  renderConfigPage(pageId.c_str());
}

// Shared POST handler for all registered config pages
void handleConfigPagePost() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  if (!s_server) return;
  String pageId = s_server->uri().substring(1);
  handleSaveConfigPage(pageId.c_str());
}

void handleExitSave() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  if (s_callbacks.saveConfig)  s_callbacks.saveConfig(s_callbacks.context);
  if (s_callbacks.applyConfig) s_callbacks.applyConfig(s_callbacks.context);
  renderExitPage(true);
  s_exitWasSave     = true;
  s_exitScheduledMs = millis(); // triggers exit after kPostExitDelayMs
}

void handleExitCancel() {
  recordActivity();
  if (!ensureAuthenticated()) return;
  // Revert in-memory changes by reloading from NVS
  if (s_callbacks.loadConfig)  s_callbacks.loadConfig(s_callbacks.context);
  if (s_callbacks.applyConfig) s_callbacks.applyConfig(s_callbacks.context);
  renderExitPage(false);
  s_exitWasSave     = false;
  s_exitScheduledMs = millis();
}

void handleLogout() {
  recordActivity();
  s_authenticated = false;
  sendRedirect("/");
}

// Captive portal: redirect OS connectivity-check requests to the portal
void handleCaptiveRedirect() {
  if (!s_server) return;
  s_server->sendHeader("Connection", "close");
  s_server->sendHeader("Location", "http://192.168.4.1/");
  s_server->send(302, "text/plain", "");
}

void handleNotFound() {
  handleCaptiveRedirect();
}

} // namespace

// ─── Public API ───────────────────────────────────────────────────────────────

void apPortalBegin() {
  s_pageCount  = 0;
  s_fieldCount = 0;
}

void apPortalEnd() {
  apPortalStopServer();
}

bool apPortalStartServer(uint16_t port, bool enableDns) {
  if (s_serverRunning) return true;

  s_serverPort = (port == 0) ? kDefaultHttpPort : port;
  s_server = new WebServer(s_serverPort);
  if (!s_server) return false;

  generateLoginPin();
  s_authenticated    = false;
  s_displayStage     = AP_STAGE_SSID;
  s_exitRequested    = false;
  s_exitWasSave      = false;
  s_exitScheduledMs  = 0;
  s_hasUnsavedChanges= false;
  s_lastActivityMs   = millis();
  s_flashMessage[0]  = '\0';

  // Core routes
  s_server->on("/",            HTTP_GET,  handleRoot);
  s_server->on("/login",       HTTP_POST, handleLogin);
  s_server->on("/menu",        HTTP_GET,  handleMenu);
  s_server->on("/info",        HTTP_GET,  handleInfo);
  s_server->on("/exit/save",   HTTP_GET,  handleExitSave);
  s_server->on("/exit/cancel", HTTP_GET,  handleExitCancel);
  s_server->on("/logout",      HTTP_GET,  handleLogout);

  // Per-page routes — registered per page at start time
  for (size_t i = 0; i < s_pageCount; ++i) {
    String path = String("/") + s_pages[i].pageId;
    s_server->on(path.c_str(), HTTP_GET,  handleConfigPageGet);
    s_server->on(path.c_str(), HTTP_POST, handleConfigPagePost);
  }

  // OS captive-portal detection endpoints
  s_server->on("/generate_204",        HTTP_GET, handleCaptiveRedirect); // Android
  s_server->on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect); // iOS / macOS
  s_server->on("/ncsi.txt",            HTTP_GET, handleCaptiveRedirect); // Windows
  s_server->on("/connecttest.txt",     HTTP_GET, handleCaptiveRedirect); // Windows 10+
  s_server->on("/redirect",            HTTP_GET, handleCaptiveRedirect); // Windows
  s_server->onNotFound(handleNotFound);

  s_server->begin();
  s_serverRunning = true;

  // DNS server — redirect all names to 192.168.4.1 for captive portal
  if (enableDns) {
    s_dns = new DNSServer();
    if (s_dns) {
      s_dns->start(kDnsPort, "*", IPAddress(192, 168, 4, 1));
    }
  }

  return true;
}

void apPortalStopServer() {
  if (s_dns) {
    s_dns->stop();
    delete s_dns;
    s_dns = nullptr;
  }
  if (s_server) {
    s_server->stop();
    delete s_server;
    s_server = nullptr;
  }
  s_serverRunning = false;
  s_authenticated = false;
}

void apPortalService() {
  if (!s_serverRunning || !s_server) return;

  if (s_dns) s_dns->processNextRequest();
  s_server->handleClient();

  // Inactivity timeout — closes AP entirely per spec §3
  if (s_lastActivityMs > 0 && !s_exitRequested && s_exitScheduledMs == 0) {
    if ((millis() - s_lastActivityMs) > kInactivityMs) {
      Serial.println("[AP-PORTAL] Inactivity timeout — closing portal");
      s_exitWasSave   = false;
      s_exitRequested = true;
    }
  }

  // Delayed exit — give browser time to receive the exit confirmation page
  if (s_exitScheduledMs > 0 && !s_exitRequested) {
    if ((millis() - s_exitScheduledMs) >= kPostExitDelayMs) {
      s_exitRequested   = true;
      s_exitScheduledMs = 0;
    }
  }
}

bool apPortalIsServerRunning() { return s_serverRunning; }

const char *apPortalGetLoginPin() { return s_loginPin; }

APDisplayStage apPortalGetDisplayStage() { return s_displayStage; }

void apPortalSignalClientConnected() {
  if (s_displayStage == AP_STAGE_SSID) s_displayStage = AP_STAGE_IP;
}

bool apPortalShouldExit()       { return s_exitRequested; }
bool apPortalExitWasSave()      { return s_exitWasSave; }
void apPortalClearExitRequest() { s_exitRequested = false; s_exitWasSave = false; s_exitScheduledMs = 0; }

bool apPortalRegisterPage(const char *pageId, const char *title) {
  if (!hasText(pageId) || !hasText(title)) return false;
  if (s_pageCount >= kMaxPages)            return false;
  if (pageExists(pageId))                  return false;
  s_pages[s_pageCount].pageId = pageId;
  s_pages[s_pageCount].title  = title;
  ++s_pageCount;
  return true;
}

bool apPortalRegisterField(const APFieldDefinition &field) {
  if (!hasText(field.pageId) || !hasText(field.fieldId) || !hasText(field.label)) return false;
  if (!pageExists(field.pageId))                         return false;
  if (field.maxLength == 0)                              return false;
  if (field.optionCount > 0 && !field.options)           return false;
  if (s_fieldCount >= kMaxFields)                        return false;
  if (fieldExists(field.fieldId))                        return false;
  s_fields[s_fieldCount].field = field;
  ++s_fieldCount;
  return true;
}

void apPortalSetCallbacks(const APPortalCallbacks &callbacks) {
  s_callbacks = callbacks;
}
