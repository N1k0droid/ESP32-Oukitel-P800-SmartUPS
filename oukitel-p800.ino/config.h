/*
 * Configuration file for Oukitel P800E Power Station
 * All system constants and pin definitions
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// System version
#define FIRMWARE_VERSION "1.2.1"
#define DEVICE_NAME "Oukitel-P800E"

// ===================================================================
// LOG LEVELS
// ===================================================================
#define LOG_LEVEL_DEBUG   0
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR   3
#define LOG_LEVEL_NONE    4

#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO  // Default log level (INFO)

// Hardware pin definitions - MAPPED BUTTONS
#define PIN_BUTTON_POWER      18    // GPIO18 - POWER button (3 seconds)
#define PIN_BUTTON_USB        16    // GPIO16 - USB output
#define PIN_BUTTON_DC         17    // GPIO17 - DC output
#define PIN_BUTTON_FLASHLIGHT 19    // GPIO19 - Flashlight
#define PIN_BUTTON_AC         21    // GPIO21 - AC output

// ADC pins
#define PIN_SCT013_MAIN       34    // ADC1_CH6 - Main current sensor (IN)
#define PIN_SCT013_OUTPUT     35    // ADC1_CH7 - Output current sensor (OUT)
#define PIN_BATTERY_VOLTAGE   36    // ADC1_CH0 - Battery voltage divider

// ADC configuration
#define ADC_RESOLUTION        4096
#define ADC_VREF             3.3

// ===================================================================
// SCT013 SENSOR CALIBRATION - DEFAULT VALUES
// ===================================================================
#define SCT013_CALIBRATION_IN_DEFAULT   28.00
#define SCT013_OFFSET_IN_DEFAULT        0.65
#define SCT013_CALIBRATION_OUT_DEFAULT  32.20
#define SCT013_OFFSET_OUT_DEFAULT       0.55
#define SCT013_SAMPLES          1480
#define MAINS_VOLTAGE           230.0  // Default mains voltage (configurable via UI)

// Warm-up configuration
#define SCT013_WARMUP_CYCLES    7
#define SCT013_AVG_SAMPLES      5

// ===================================================================
// BATTERY VOLTAGE DIVIDER CONFIGURATION - DEFAULT VALUES
// ===================================================================
#define BATTERY_R1              220000.0
#define BATTERY_R2              27000.0
#define BATTERY_ADC_SAMPLES     100
#define BATTERY_ADC_CALIBRATION_DEFAULT 1.0125
#define BATTERY_DIVIDER_RATIO_DEFAULT   8.925

// Battery voltage range
#define BATTERY_VMAX            29.0
#define BATTERY_VMIN            20.0
#define BATTERY_CAPACITY_AH     100.0

// ===================================================================
// VOLTAGE COMPENSATION OFFSETS - DEFAULT VALUES
// ===================================================================
#define VOLTAGE_OFFSET_CHARGE_DEFAULT      0.00
#define VOLTAGE_OFFSET_DISCHARGE_DEFAULT   0.00
#define VOLTAGE_OFFSET_REST_DEFAULT       -0.20
#define VOLTAGE_OFFSET_BYPASS             -0.20

// Power offsets
#define POWER_IN_OFFSET           0.0
#define POWER_OUT_OFFSET          0.0

// ===================================================================
// ADVANCED SETTINGS - DEFAULT VALUES (NEW!)
// ===================================================================
#define POWER_THRESHOLD_DEFAULT           10.0
#define POWER_FILTER_ALPHA_DEFAULT        0.2
#define VOLTAGE_MIN_SAFE_DEFAULT          23.5
#define BATTERY_LOW_WARNING_DEFAULT       20.0
#define BATTERY_CRITICAL_DEFAULT          10.0
#define AUTO_POWER_ON_DELAY_DEFAULT       10000
#define SOC_BUFFER_SIZE_DEFAULT           10
#define SOC_CHANGE_THRESHOLD_DEFAULT      3
#define POWER_STATION_OFF_VOLTAGE_DEFAULT 20.0
#define WARMUP_DELAY_DEFAULT              30000  // 30 seconds warmup delay
#define MAX_POWER_READING_DEFAULT         1700.0 // Maximum valid power reading in Watts

// ===================================================================
// HTTP API SECURITY - DEFAULT VALUES
// ===================================================================
#define API_PASSWORD_DEFAULT              "oukitel2025"
#define API_PASSWORD_FILE                 "/api_password.txt"

// ===================================================================
// NTP CONFIGURATION - DEFAULT VALUES (NEW!)
// ===================================================================
#define NTP_SERVER_DEFAULT                "pool.ntp.org"
#define NTP_GMT_OFFSET_DEFAULT            3600      // GMT+1 for CET
#define NTP_DAYLIGHT_OFFSET_DEFAULT       3600      // +1 hour for CEST

// ===================================================================
// HTTP SHUTDOWN NOTIFICATION - DEFAULT VALUES (NEW!)
// ===================================================================
#define HTTP_SHUTDOWN_THRESHOLD_DEFAULT   15.0      // Battery % threshold
#define HTTP_SHUTDOWN_SERVER_DEFAULT      ""
#define HTTP_SHUTDOWN_PORT_DEFAULT        8080
#define HTTP_SHUTDOWN_PASSWORD_DEFAULT    "shutdown123"

// ===================================================================
// BATTERY STATE DEFINITIONS (DEFINE FIRST!)
// ===================================================================
enum BatteryState {
  STATE_REST,
  STATE_CHARGING,
  STATE_DISCHARGING,
  STATE_BYPASS
};

enum SystemStatus {
  STATUS_INITIALIZING,
  STATUS_NORMAL,
  STATUS_ON_BATTERY,
  STATUS_LOW_BATTERY,
  STATUS_CRITICAL_BATTERY,
  STATUS_ERROR
};

enum ButtonState {
  BUTTON_OFF = 0,
  BUTTON_ON = 1,
  BUTTON_UNKNOWN = 2
};

enum ButtonIndex {
  BTN_POWER = 0,
  BTN_USB = 1,
  BTN_DC = 2,
  BTN_FLASHLIGHT = 3,
  BTN_AC = 4
};

// ===================================================================
// DATA STRUCTURES (Now can use enums defined above)
// ===================================================================

struct CalibrationData {
  float sct013CalIn;
  float sct013OffsetIn;
  float sct013CalOut;
  float sct013OffsetOut;
  float batteryDividerRatio;
  float batteryAdcCalibration;
  float voltageOffsetCharge;
  float voltageOffsetDischarge;
  float voltageOffsetRest;
  float fixedVoltage;        // NEW: Override measured battery voltage (0 = disabled)
  float mainsVoltage;        // NEW: Configurable mains voltage for IN/OUT power calc
  bool valid;
};

struct AdvancedSettings {
  float powerThreshold;
  float powerFilterAlpha;
  float voltageMinSafe;
  float batteryLowWarning;
  float batteryCritical;
  uint32_t autoPowerOnDelay;
  int socBufferSize;
  int socChangeThreshold;
  float powerStationOffVoltage;
  uint32_t warmupDelay;
  float maxPowerReading;
  bool valid;
};

struct SystemSettings {
  String ntpServer;
  int32_t gmtOffset;
  int32_t daylightOffset;
  bool beepsEnabled;  // Global switch to enable/disable all beeps
  int logLevel;       // Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=NONE)
  bool valid;
};

struct HttpShutdownConfig {
  bool enabled;
  float batteryThreshold;
  String server;
  int port;
  String password;
  bool shutdownSent;  // Track if shutdown notification was already sent
  bool valid;
};

struct SensorData {
  float mainCurrent;
  float outputCurrent;
  float batteryVoltage;
  float batteryPercentage;
  float mainPower;
  float outputPower;
  bool onBattery;
  BatteryState batteryState;
  unsigned long timestamp;
};

struct EnergyData {
  float dailyConsumption;
  float monthlyConsumption;
  float instantPower;
  float peakPower;
  unsigned long operatingTime;
};

struct MonthlyEnergyRecord {
  int year;
  int month;
  float consumption;
};

struct WiFiCredentials {
  String ssid;
  String password;
  bool valid;
};

struct MQTTConfig {
  String server;
  int port;
  String username;
  String password;
  String clientId;
  bool enabled;
};

struct HTTPConfig {
  String server;
  int port;
  String endpoint;
  String apiKey;
  bool enabled;
};

struct UPSConfig {
  bool enabled;
  int port;
  int shutdownThreshold;
};

struct APIConfig {
  String password;
  bool enabled;
};

struct SystemConfig {
  WiFiCredentials wifi;
  MQTTConfig mqtt;
  HTTPConfig http;
  UPSConfig ups;
  APIConfig api;
  String deviceName;
  bool autoPowerOnEnabled;
};

// ===================================================================
// EXTERNAL CALIBRATION VARIABLES (loaded from SPIFFS at boot)
// ===================================================================
extern float g_sct013CalIn;
extern float g_sct013OffsetIn;
extern float g_sct013CalOut;
extern float g_sct013OffsetOut;
extern float g_batteryDividerRatio;
extern float g_batteryAdcCalibration;
extern float g_voltageOffsetCharge;
extern float g_voltageOffsetDischarge;
extern float g_voltageOffsetRest;
extern float g_fixedVoltage;             // Battery voltage override
extern float g_mainsVoltage;             // Configurable mains voltage

// ===================================================================
// EXTERNAL ADVANCED SETTINGS VARIABLES (loaded from SPIFFS at boot)
// ===================================================================
extern float g_powerThreshold;
extern float g_powerFilterAlpha;
extern float g_voltageMinSafe;
extern float g_batteryLowWarning;
extern float g_batteryCritical;
extern uint32_t g_autoPowerOnDelay;
extern int g_socBufferSize;
extern int g_socChangeThreshold;
extern float g_powerStationOffVoltage;
extern uint32_t g_warmupDelay;
extern float g_maxPowerReading;

// ===================================================================
// EXTERNAL API PASSWORD VARIABLE
// ===================================================================
extern String g_apiPassword;

// ===================================================================
// EXTERNAL SYSTEM SETTINGS VARIABLES (NEW!)
// ===================================================================
extern String g_ntpServer;
extern int32_t g_gmtOffset;
extern int32_t g_daylightOffset;
extern bool g_beepsEnabled;
extern int g_logLevel;

// ===================================================================
// EXTERNAL HTTP SHUTDOWN CONFIG VARIABLES (NEW!)
// ===================================================================
extern bool g_httpShutdownEnabled;
extern float g_httpShutdownThreshold;
extern String g_httpShutdownServer;
extern int g_httpShutdownPort;
extern String g_httpShutdownPassword;
extern bool g_httpShutdownSent;

// Function declarations for calibration management
extern void loadCalibrationFromSPIFFS();
extern void saveCalibrationToSPIFFS(const CalibrationData& cal);
extern void loadAdvancedSettingsFromSPIFFS();
extern void saveAdvancedSettingsToSPIFFS(const AdvancedSettings& settings);
extern void loadAPIPasswordFromSPIFFS();
extern void saveAPIPasswordToSPIFFS(const String& password);
extern void loadSystemSettingsFromSPIFFS();
extern void saveSystemSettingsToSPIFFS(const SystemSettings& settings);
extern void loadHttpShutdownConfigFromSPIFFS();
extern void saveHttpShutdownConfigToSPIFFS(const HttpShutdownConfig& config);

// ===================================================================
// FILTERING CONFIGURATION (These will be overridden by dynamic values)
// ===================================================================
#define SOC_BUFFER_SIZE           10
#define SOC_CHANGE_THRESHOLD      3
#define POWER_FILTER_ALPHA        0.3

// ===================================================================
// THRESHOLDS AND LIMITS (These will be overridden by dynamic values)
// ===================================================================
#define POWER_THRESHOLD           10.0
#define VOLTAGE_MIN_SAFE          23.5
#define BATTERY_LOW_WARNING       20.0
#define BATTERY_CRITICAL          10.0
#define POWER_STATION_OFF_VOLTAGE 20.0

// ===================================================================
// BUTTON CONFIGURATION
// ===================================================================
#define BUTTON_DEBOUNCE_MS        50
#define BUTTON_POWER_DURATION     3000
#define BUTTON_STANDARD_DURATION  1000
#define FLASHLIGHT_ALERT_PULSES   4
#define FLASHLIGHT_ALERT_INTERVAL 500
#define AUTO_POWER_ON_FILE        "/autopoweron.txt"
#define ADVANCED_SETTINGS_FILE    "/advanced.json"
#define SYSTEM_SETTINGS_FILE      "/system.json"
#define HTTP_SHUTDOWN_CONFIG_FILE "/http_shutdown.json"

// ===================================================================
// WIFI CONFIGURATION
// ===================================================================
#define WIFI_CONNECT_TIMEOUT      10000
#define WIFI_RETRY_ATTEMPTS       3
#define WIFI_AP_TIMEOUT           300000
#define AP_SSID                   "Oukitel-P800A"
#define AP_PASSWORD               ""
#define AP_CHANNEL                1
#define AP_MAX_CONNECTIONS        4
// ===================================================================
// DEFAULT WIFI CREDENTIALS - DEVELOPMENT ONLY
// ⚠️ WARNING: These are for development/testing only!
// ⚠️ Remove or change these before production deployment
// ⚠️ For production, use the web interface to configure WiFi
// ===================================================================
#define DEFAULT_WIFI_SSID     "SET-YOUR-SSID"
#define DEFAULT_WIFI_PASSWORD "SET-PASSWORD"

// Web server configuration
#define WEB_SERVER_PORT           80
#define WEBSOCKET_PORT            81
#define WEB_USERNAME              "admin"
#define WEB_PASSWORD              "P800e"

// UPS protocol configuration
#define UPS_PORT                  3493
#define UPS_MAX_CLIENTS           5
#define UPS_TIMEOUT               30000
#define UPS_SHUTDOWN_THRESHOLD    20

// MQTT configuration
#define MQTT_PORT                 1883
#define MQTT_KEEPALIVE            60
#define MQTT_QOS                  1
#define MQTT_RETAIN               true
#define MQTT_TOPIC_PREFIX         "oukitel_p800e"

// Data logging configuration
#define LOG_INTERVAL              60000
#define LOG_RETENTION_DAYS        30
#define ENERGY_CALC_INTERVAL      1000
#define SENSOR_UPDATE_INTERVAL    1000
#define DATA_LOG_INTERVAL         300000
#define SENSOR_READ_INTERVAL      1000
#define SERIAL_LOG_INTERVAL       30000
#define SERIAL_LOG_HEADER_INTERVAL 300000

// EEPROM/SPIFFS configuration
#define EEPROM_SIZE               4096
#define CONFIG_FILE               "/config.json"
#define WIFI_CREDS_FILE           "/wifi.json"
#define ENERGY_LOG_FILE           "/energy.log"
#define CALIBRATION_FILE          "/calibration.json"
#define ENERGY_HISTORY_FILE       "/energy_history.json"

// ===================================================================
// BATTERY VOLTAGE-SOC CURVES (VALIDATED - DO NOT MODIFY)
// ===================================================================

const float BATTERY_CURVE_NORMAL[][2] = {
  {100.0, 26.70}, {95.0, 26.60}, {90.0, 26.50}, {80.0, 26.05},
  {75.0, 25.99}, {70.0, 25.95}, {65.0, 25.95}, {60.0, 25.90},
  {50.0, 25.77}, {45.0, 25.75}, {40.0, 25.70}, {35.0, 25.60},
  {30.0, 25.50}, {25.0, 25.35}, {20.0, 25.15}, {15.0, 25.00},
  {10.0, 24.50}, {5.0, 23.90}, {4.0, 23.66}, {3.0, 23.50},
  {2.0, 23.40}, {1.0, 23.30}, {0.0, 22.70}
};
const int CURVE_NORMAL_POINTS = 23;

const float BATTERY_CURVE_CHARGE[][2] = {
  {100.0, 26.70}, {95.0, 26.60}, {90.0, 26.50}, {80.0, 26.05},
  {75.0, 25.99}, {70.0, 25.95}, {65.0, 25.95}, {60.0, 25.90},
  {50.0, 25.77}, {45.0, 25.75}, {40.0, 25.70}, {35.0, 25.60},
  {30.0, 25.50}, {25.0, 25.35}, {20.0, 25.25}, {15.0, 25.15},
  {5.0, 25.00}, {0.0, 24.00}
};
const int CURVE_CHARGE_POINTS = 18;

#endif // CONFIG_H