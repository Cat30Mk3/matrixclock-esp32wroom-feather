#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <Ticker.h>
#include <Timezone.h>
#include <TimeLib.h>

// ============================================================================
// FORWARD DECLARATIONS - Struct defined in secrets.h / secrets_template.h
// ============================================================================
typedef struct {
  char mqttDeviceName[3][40];
  char ssid[30];
  char password[30];
  char smtpServer[20];
  char smtpUserId[35];
  char smtpPassword[20];
  char sendingAddress[35];
  char receivingAddress[35];
  char mqttServer[35];
  char mqttUserId[100];
  char mqttClientId[35];
  char mqttPassword[35];
  char lampCmndTopic[20];
  char lampStatTopic[20];
} configDb_t;

// Declare configDb - defined in globals.cpp (includes secrets.h)
extern configDb_t configDb;

// ============================================================================
// COMPILER MACROS - DISPLAY CONFIGURATION
// ============================================================================
#define DISPLAY1X4 0
#define DISPLAY2X8 1
#define DISPLAY_CONFIG DISPLAY2X8  // <<< SELECT DISPLAY CONFIGURATION

#define SWAP_DS18B20 1              // 0 for no swap, 1 for swap
#define ONE_TEMP_IS_IN 1            // 0 for "ONE_TEMP_IS_OUT", 1 for "ONE_TEMP_IS_IN"

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW

#if DISPLAY_CONFIG == DISPLAY1X4
  #define MAX_ZONES_FULL 1
  #define MAX_ZONES_HALF 0
  #define ZONE_SIZE_FULL 4
  #define ZONE_SIZE_HALF 4
  #define ZONE_SINGLE 0
  #define HDOTS_PER_CHAR 6
  #define VDOTS_PER_CHAR 8
  #define H_PAUSE  0
  #define V_PAUSE  2000
  #define H_SCROLL_SPEED  25
  #define V_SCROLL_SPEED  80
#elif DISPLAY_CONFIG == DISPLAY2X8
  #define MAX_ZONES_FULL 2
  #define MAX_ZONES_HALF 4
  #define ZONE_SIZE_FULL 8
  #define ZONE_SIZE_HALF 4
  #define ZONE_UPPER   0
  #define ZONE_LOWER   1
  #define ZONE_UP_LFT  2
  #define ZONE_UP_RGT  3
  #define ZONE_DN_LFT  4
  #define ZONE_DN_RGT  5
  #define HDOTS_PER_CHAR 12
  #define VDOTS_PER_CHAR 8
  #define H_PAUSE  0
  #define V_PAUSE  1500
  #define H_SCROLL_SPEED   25
  #define V_SCROLL_SPEED  75
#endif

#define HDOTS_PER_DISP HDOTS_PER_CHAR * ZONE_SIZE_FULL
#define VDOTS_PER_DISP VDOTS_PER_CHAR
#define MAX_DEVICES (MAX_ZONES_FULL * ZONE_SIZE_FULL)

// GPIO Pin Definitions
#define CLK_PIN       5    // SPI BUS - CLK
#define DATA_PIN     18    // SPI BUS - MOSI
#define CS_PIN       21    // SPI BUS - CS for MD_MAX7219

#define BLU_LED_PIN 15    // GPIO - output
#define RED_LED_PIN 33    // GPIO - output
#define YEL_LED_PIN 27    // GPIO - output
#define LAMP_PIN    14    // GPIO - output

#define PB_SEL_PIN 36    // GPIO - input
#define PB_DEC_PIN 39    // GPIO - input
#define PB_INC_PIN 34    // GPIO - input
#define PB_CNL_PIN 25    // GPIO - input
#define PB_MEN_PIN 26    // GPIO - input

#define DS18B20_PIN A5   // Dallas One Wire Bus for temperature sensor(s)

// ============================================================================
// DISPLAY PARAMETER INDICES
// ============================================================================
#define DISP_CURR_TIME              0
#define DISP_CURR_DOW               1
#define DISP_CURR_DATE              2
#define DISP_CURR_WIRED_TEMP_IN     3
#define DISP_CURR_WIRED_TEMP_OUT    4
#define DISP_CURR_MQTT_COT_TEMP_IN  5
#define DISP_CURR_MQTT_COT_TEMP_OUT 6
#define DISP_CURR_MQTT_COT_TEMP_CRL 7
#define DISP_CURR_MQTT_COT_TEMP_SMP 8
#define DISP_CURR_MQTT_COT_MSG      9
#define DISP_CURR_MQTT_HOM_TEMP_IN  10
#define DISP_CURR_MQTT_HOM_TEMP_OUT 11
#define DISP_CURR_MQTT_HOM_TEMP_ATT 12
#define DISP_CURR_MQTT_HOM_TEMP_CAV 13
#define DISP_CURR_MQTT_HOM_MSG      14
#define DISP_PARAM_TOTAL_COUNT      15

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
#define MQTT_TELE_TOPIC_TIMEOUT_MS 240000L

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Configuration database structure
// typedef struct {
//   char mqttDeviceName[3][40];
//   char ssid[30];
//   char password[30];
//   char smtpServer[20];
//   char smtpUserId[35];
//   char smtpPassword[20];
//   char sendingAddress[35];
//   char receivingAddress[35];
//   char mqttServer[35];
//   char mqttUserId[100];
//   char mqttClientId[35];
//   char mqttPassword[35];
//   char lampCmndTopic[20];
//   char lampStatTopic[20];
// } configDb_t;

extern configDb_t configDb;

// Display parameter structure
extern struct dispParamStruct {
  int groupMode;
  boolean enabled;
  int remoteMqttDeviceIndex;
  int source;
  char mqttTopic[10];
  int symbolIndex;
  char dispBuffer[30];
  boolean dispReady;
  long lastReceivedUpdate;
} dispParam[DISP_PARAM_TOTAL_COUNT];

// ============================================================================
// GLOBAL VARIABLES - VERSIONING
// ============================================================================
extern char projectNameFromFileName[50];
extern char projectVersionFromFileName[20];
extern char compileDateFromFileName[15];
extern char compileTimeFromFileName[12];

// ============================================================================
// GLOBAL VARIABLES - NTP & TIME
// ============================================================================
extern const char ntpServerName[];
extern const int timeZone;
extern unsigned int localPort;
extern Timezone myTZ;
extern TimeChangeRule myTCR;

// ============================================================================
// GLOBAL VARIABLES - PAROLA DISPLAY
// ============================================================================
extern MD_Parola parola;
extern MD_MAX72XX mx;
extern int singleCharSpace;
extern int doubleCharSpace;
extern char dispBuffer[15][30];
extern int statusLinePosition;
extern long int cycleTimer;
extern int cycleCounter;
extern boolean cycleEnabled[9];
extern bool invertUpperZone;

// ============================================================================
// GLOBAL VARIABLES - MQTT
// ============================================================================
extern PubSubClient mqttClient;
extern WiFiClient espClient;
extern char msg[50];
extern volatile bool mqttLampStatPubFlag;
extern volatile bool mqttAlive;
extern volatile bool mqttCallbackInprogress;

// ============================================================================
// GLOBAL VARIABLES - TICKER
// ============================================================================
extern Ticker tickerBlinkerInstance;
extern Ticker tickerDelayInstance;
extern Ticker tickerMqttKeepAliveInstance;
extern Ticker tickerTempStartInstance;
extern Ticker tickerTempGetInstance;
extern volatile boolean delayFlag;

// ============================================================================
// GLOBAL VARIABLES - ISR
// ============================================================================
extern volatile int isrFlag;
extern volatile boolean msgDisplayRequested;
extern volatile boolean displayNormal;

// ============================================================================
// GLOBAL VARIABLES - DATE & TIME
// ============================================================================
extern char daysOfTheWeek[7][4];
extern char months[12][4];

// ============================================================================
// GLOBAL VARIABLES - DS18B20 TEMPERATURE
// ============================================================================
extern int ds18b20Count;
extern boolean indoorIsZeroIndex;

// ============================================================================
// GLOBAL VARIABLES - LAMP CONTROL
// ============================================================================
extern volatile boolean lamp_1_State;

#endif // GLOBALS_H
