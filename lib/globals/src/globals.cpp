#include "globals.h"
#include <WiFi.h>
#include <PubSubClient.h>

// Include secrets (real or template) to get configDb definition
#if __has_include("secrets.h")
  #include "secrets.h"
  #define SECRETS_IS_TEMPLATE 0
#elif __has_include("secrets_template.h")
  #include "secrets_template.h"
  #define SECRETS_IS_TEMPLATE 1
  #warning "Using dummy secrets from secrets_template.h. Create include/secrets.h for full functionality."
#else
  #error "Missing secrets header. Add include/secrets_template.h (public) or include/secrets.h (private)."
#endif


// ============================================================================
// DISPLAY PARAMETER ARRAY
// ============================================================================
dispParamStruct dispParam[DISP_PARAM_TOTAL_COUNT];

// ============================================================================
// VERSIONING GLOBALS
// ============================================================================
char projectNameFromFileName[50];
char projectVersionFromFileName[20];
char compileDateFromFileName[15];
char compileTimeFromFileName[12];

// ============================================================================
// NTP & TIME GLOBALS
// ============================================================================
const char ntpServerName[] = "pool.ntp.org";
const int timeZone = 0;
unsigned int localPort = 8888;

// US Eastern Time (EST/EDT)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC-4
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC-5
Timezone myTZ(usEDT, usEST);
TimeChangeRule myTCR;

// ============================================================================
// PAROLA DISPLAY GLOBALS
// ============================================================================
// These are instantiated in main.cpp
// MD_Parola parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
// MD_MAX72XX mx(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
int singleCharSpace;
int doubleCharSpace;
char dispBuffer[15][30];
int statusLinePosition = 0;
long int cycleTimer;
int cycleCounter = 0;
boolean cycleEnabled[9] = {true, true, false, false, false, false, false, false, false};
bool invertUpperZone = false;

// ============================================================================
// MQTT GLOBALS
// ============================================================================
// WiFiClient espClient;  // Instantiated in main.cpp
// PubSubClient mqttClient(espClient);  // Instantiated in main.cpp
char msg[50];
volatile bool mqttLampStatPubFlag = false;
volatile bool mqttAlive = false;
volatile bool mqttCallbackInprogress = false;

// ============================================================================
// TICKER GLOBALS
// ============================================================================
Ticker tickerBlinkerInstance;
Ticker tickerDelayInstance;
Ticker tickerMqttKeepAliveInstance;
Ticker tickerTempStartInstance;
Ticker tickerTempGetInstance;
volatile boolean delayFlag = false;

// ============================================================================
// ISR GLOBALS
// ============================================================================
volatile int isrFlag = -1;
volatile boolean msgDisplayRequested = false;
volatile boolean displayNormal = true;

// ============================================================================
// DATE & TIME GLOBALS
// ============================================================================
char daysOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// ============================================================================
// DS18B20 GLOBALS
// ============================================================================
int ds18b20Count = 0;
boolean indoorIsZeroIndex = true;

// ============================================================================
// LAMP CONTROL GLOBALS
// ============================================================================
volatile boolean lamp_1_State = false;
