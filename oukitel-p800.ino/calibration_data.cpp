/*
 * Calibration Data Management
 * Handles persistent storage of calibration values in SPIFFS
 */

#include "config.h"
#include "logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ===================================================================
// CALIBRATION VARIABLES (loaded from SPIFFS at boot)
// ===================================================================
float g_sct013CalIn = SCT013_CALIBRATION_IN_DEFAULT;
float g_sct013OffsetIn = SCT013_OFFSET_IN_DEFAULT;
float g_sct013CalOut = SCT013_CALIBRATION_OUT_DEFAULT;
float g_sct013OffsetOut = SCT013_OFFSET_OUT_DEFAULT;
float g_batteryDividerRatio = BATTERY_DIVIDER_RATIO_DEFAULT;
float g_batteryAdcCalibration = BATTERY_ADC_CALIBRATION_DEFAULT;
float g_voltageOffsetCharge = VOLTAGE_OFFSET_CHARGE_DEFAULT;
float g_voltageOffsetDischarge = VOLTAGE_OFFSET_DISCHARGE_DEFAULT;
float g_voltageOffsetRest = VOLTAGE_OFFSET_REST_DEFAULT;
float g_fixedVoltage = 0.0;            // Override measured voltage (0 = disabled)
float g_mainsVoltage = MAINS_VOLTAGE;  // Configurable mains voltage

// ===================================================================
// ADVANCED SETTINGS VARIABLES (loaded from SPIFFS at boot)
// ===================================================================
float g_powerThreshold = POWER_THRESHOLD_DEFAULT;
float g_powerFilterAlpha = POWER_FILTER_ALPHA_DEFAULT;
float g_voltageMinSafe = VOLTAGE_MIN_SAFE_DEFAULT;
float g_batteryLowWarning = BATTERY_LOW_WARNING_DEFAULT;
float g_batteryCritical = BATTERY_CRITICAL_DEFAULT;
uint32_t g_autoPowerOnDelay = AUTO_POWER_ON_DELAY_DEFAULT;
int g_socBufferSize = SOC_BUFFER_SIZE_DEFAULT;
int g_socChangeThreshold = SOC_CHANGE_THRESHOLD_DEFAULT;
float g_powerStationOffVoltage = POWER_STATION_OFF_VOLTAGE_DEFAULT;
uint32_t g_warmupDelay = WARMUP_DELAY_DEFAULT;
float g_maxPowerReading = MAX_POWER_READING_DEFAULT;

// ===================================================================
// API PASSWORD VARIABLE (loaded from SPIFFS at boot)
// ===================================================================
String g_apiPassword = API_PASSWORD_DEFAULT;

// ===================================================================
// SYSTEM SETTINGS VARIABLES (NEW!)
// ===================================================================
String g_ntpServer = NTP_SERVER_DEFAULT;
int32_t g_gmtOffset = NTP_GMT_OFFSET_DEFAULT;
int32_t g_daylightOffset = NTP_DAYLIGHT_OFFSET_DEFAULT;
bool g_beepsEnabled = true;  // Beeps are enabled by default
int g_logLevel = LOG_LEVEL_DEFAULT;  // Default log level (INFO)

// ===================================================================
// HTTP SHUTDOWN CONFIG VARIABLES (NEW!)
// ===================================================================
bool g_httpShutdownEnabled = false;
float g_httpShutdownThreshold = HTTP_SHUTDOWN_THRESHOLD_DEFAULT;
String g_httpShutdownServer = HTTP_SHUTDOWN_SERVER_DEFAULT;
int g_httpShutdownPort = HTTP_SHUTDOWN_PORT_DEFAULT;
String g_httpShutdownPassword = HTTP_SHUTDOWN_PASSWORD_DEFAULT;
bool g_httpShutdownSent = false;

// ===================================================================
// CALIBRATION FUNCTIONS
// ===================================================================

void loadCalibrationFromSPIFFS() {
  Serial.println("[CAL] Loading calibration from SPIFFS...");
  
  if (!SPIFFS.exists(CALIBRATION_FILE)) {
    Serial.println("[CAL] No calibration file found, using defaults");
    Serial.println("[CAL] Default values:");
    Serial.println("     SCT013 Cal In: " + String(g_sct013CalIn, 2));
    Serial.println("     Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
    Serial.println("     Fixed Voltage: " + String(g_fixedVoltage, 1) + "V");
    Serial.println("     Mains Voltage: " + String(g_mainsVoltage, 1) + "V");
    return;
  }

  File configFile = SPIFFS.open(CALIBRATION_FILE, "r");
  if (!configFile) {
    LOG_ERROR("Calibration: Failed to open file for reading");
    Serial.println("[CAL] SPIFFS may be corrupted or file system error");
    Serial.println("[CAL] Using default values - calibration may need to be reconfigured");
    return;
  }
  
  // Check file size to prevent reading corrupted files
  size_t fileSize = configFile.size();
  if (fileSize == 0 || fileSize > 2048) {
    LOG_ERROR("Calibration: File size invalid (" + String(fileSize) + " bytes)");
    configFile.close();
    Serial.println("[CAL] Using default values");
    return;
  }

  DynamicJsonDocument doc(1024);  // Increased size for safety
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    LOG_ERROR("Calibration: Failed to parse JSON: " + String(error.c_str()));
    Serial.println("[CAL] File may be corrupted - using default values");
    Serial.println("[CAL] Calibration may need to be reconfigured");
    return;
  }

  // Carica i valori dal file, con fallback ai default
  g_sct013CalIn = doc["sct013CalIn"] | SCT013_CALIBRATION_IN_DEFAULT;
  g_sct013OffsetIn = doc["sct013OffsetIn"] | SCT013_OFFSET_IN_DEFAULT;
  g_sct013CalOut = doc["sct013CalOut"] | SCT013_CALIBRATION_OUT_DEFAULT;
  g_sct013OffsetOut = doc["sct013OffsetOut"] | SCT013_OFFSET_OUT_DEFAULT;
  g_batteryDividerRatio = doc["batteryDividerRatio"] | BATTERY_DIVIDER_RATIO_DEFAULT;
  g_batteryAdcCalibration = doc["batteryAdcCalibration"] | BATTERY_ADC_CALIBRATION_DEFAULT;
  g_voltageOffsetCharge = doc["voltageOffsetCharge"] | VOLTAGE_OFFSET_CHARGE_DEFAULT;
  g_voltageOffsetDischarge = doc["voltageOffsetDischarge"] | VOLTAGE_OFFSET_DISCHARGE_DEFAULT;
  g_voltageOffsetRest = doc["voltageOffsetRest"] | VOLTAGE_OFFSET_REST_DEFAULT;
  g_fixedVoltage = doc["fixedVoltage"] | 0.0;
  g_mainsVoltage = doc["mainsVoltage"] | MAINS_VOLTAGE;

  Serial.println("[CAL] Calibration loaded from SPIFFS:");
  Serial.println("     SCT013 Cal In: " + String(g_sct013CalIn, 2));
  Serial.println("     SCT013 Cal Out: " + String(g_sct013CalOut, 2));
  Serial.println("     Battery Divider Ratio: " + String(g_batteryDividerRatio, 3));
  Serial.println("     Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
  Serial.println("     Fixed Voltage: " + String(g_fixedVoltage, 1) + "V");
  Serial.println("     Mains Voltage: " + String(g_mainsVoltage, 1) + "V");
}

void saveCalibrationToSPIFFS(const CalibrationData& cal) {
  Serial.println("[CAL] Saving calibration to SPIFFS...");
  
  DynamicJsonDocument doc(896);
  doc["sct013CalIn"] = cal.sct013CalIn;
  doc["sct013OffsetIn"] = cal.sct013OffsetIn;
  doc["sct013CalOut"] = cal.sct013CalOut;
  doc["sct013OffsetOut"] = cal.sct013OffsetOut;
  doc["batteryDividerRatio"] = cal.batteryDividerRatio;
  doc["batteryAdcCalibration"] = cal.batteryAdcCalibration;
  doc["voltageOffsetCharge"] = cal.voltageOffsetCharge;
  doc["voltageOffsetDischarge"] = cal.voltageOffsetDischarge;
  doc["voltageOffsetRest"] = cal.voltageOffsetRest;
  doc["fixedVoltage"] = cal.fixedVoltage;
  doc["mainsVoltage"] = cal.mainsVoltage;

  // Check SPIFFS free space before writing
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  size_t freeBytes = totalBytes - usedBytes;
  
  if (freeBytes < 512) {
    LOG_ERROR("Calibration: Insufficient SPIFFS space (" + String(freeBytes) + " bytes free)");
    Serial.println("[CAL] Calibration not saved - please free up space");
    return;
  }
  
  File configFile = SPIFFS.open(CALIBRATION_FILE, "w");
  if (!configFile) {
    LOG_ERROR("Calibration: Failed to open file for writing");
    Serial.println("[CAL] SPIFFS may be full or corrupted");
    return;
  }

  serializeJson(doc, configFile);
  configFile.close();

  // Aggiorna le variabili globali - CRITICAL SECTION
  // Disable interrupts briefly to ensure atomic updates
  // Note: On ESP32, float assignments are typically atomic, but we ensure consistency
  noInterrupts();
  g_sct013CalIn = cal.sct013CalIn;
  g_sct013OffsetIn = cal.sct013OffsetIn;
  g_sct013CalOut = cal.sct013CalOut;
  g_sct013OffsetOut = cal.sct013OffsetOut;
  g_batteryDividerRatio = cal.batteryDividerRatio;
  g_batteryAdcCalibration = cal.batteryAdcCalibration;
  g_voltageOffsetCharge = cal.voltageOffsetCharge;
  g_voltageOffsetDischarge = cal.voltageOffsetDischarge;
  g_voltageOffsetRest = cal.voltageOffsetRest;
  g_fixedVoltage = cal.fixedVoltage;
  g_mainsVoltage = cal.mainsVoltage;
  interrupts();

  Serial.println("[CAL] Calibration saved successfully:");
  Serial.println("     SCT013 Cal In: " + String(g_sct013CalIn, 2));
  Serial.println("     Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
  Serial.println("     Fixed Voltage: " + String(g_fixedVoltage, 1) + "V");
  Serial.println("     Mains Voltage: " + String(g_mainsVoltage, 1) + "V");
}

// ===================================================================
// ADVANCED SETTINGS FUNCTIONS
// ===================================================================

void loadAdvancedSettingsFromSPIFFS() {
  Serial.println("[ADV] Loading advanced settings from SPIFFS...");
  
  if (!SPIFFS.exists(ADVANCED_SETTINGS_FILE)) {
    Serial.println("[ADV] No advanced settings file found, using defaults");
    Serial.println("[ADV] Default values:");
    Serial.println("     Power Threshold: " + String(g_powerThreshold, 2) + "W");
    Serial.println("     Power Filter Alpha: " + String(g_powerFilterAlpha, 2));
    Serial.println("     Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 2) + "V");
    Serial.println("     Auto Power On Delay: " + String(g_autoPowerOnDelay) + "ms");
    Serial.println("     Warmup Delay: " + String(g_warmupDelay) + "ms");
    Serial.println("     Max Power Reading: " + String(g_maxPowerReading, 1) + "W");
    return;
  }

  File configFile = SPIFFS.open(ADVANCED_SETTINGS_FILE, "r");
  if (!configFile) {
    LOG_ERROR("Advanced settings: Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    LOG_ERROR("Advanced settings: Failed to parse JSON: " + String(error.c_str()));
    return;
  }

  // Carica i valori dal file, con fallback ai default
  g_powerThreshold = doc["powerThreshold"] | POWER_THRESHOLD_DEFAULT;
  g_powerFilterAlpha = doc["powerFilterAlpha"] | POWER_FILTER_ALPHA_DEFAULT;
  g_voltageMinSafe = doc["voltageMinSafe"] | VOLTAGE_MIN_SAFE_DEFAULT;
  g_batteryLowWarning = doc["batteryLowWarning"] | BATTERY_LOW_WARNING_DEFAULT;
  g_batteryCritical = doc["batteryCritical"] | BATTERY_CRITICAL_DEFAULT;
  g_autoPowerOnDelay = doc["autoPowerOnDelay"] | AUTO_POWER_ON_DELAY_DEFAULT;
  g_socBufferSize = doc["socBufferSize"] | SOC_BUFFER_SIZE_DEFAULT;
  g_socChangeThreshold = doc["socChangeThreshold"] | SOC_CHANGE_THRESHOLD_DEFAULT;
  g_powerStationOffVoltage = doc["powerStationOffVoltage"] | POWER_STATION_OFF_VOLTAGE_DEFAULT;
  g_warmupDelay = doc["warmupDelay"] | WARMUP_DELAY_DEFAULT;
  g_maxPowerReading = doc["maxPowerReading"] | MAX_POWER_READING_DEFAULT;

  Serial.println("[ADV] Advanced settings loaded from SPIFFS:");
  Serial.println("     Power Threshold: " + String(g_powerThreshold, 2) + "W");
  Serial.println("     Power Filter Alpha: " + String(g_powerFilterAlpha, 2));
  Serial.println("     Voltage Min Safe: " + String(g_voltageMinSafe, 2) + "V");
  Serial.println("     Battery Low Warning: " + String(g_batteryLowWarning, 1) + "%");
  Serial.println("     Battery Critical: " + String(g_batteryCritical, 1) + "%");
  Serial.println("     Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 2) + "V");
  Serial.println("     Auto Power On Delay: " + String(g_autoPowerOnDelay) + "ms");
  Serial.println("     SOC Buffer Size: " + String(g_socBufferSize));
  Serial.println("     SOC Change Threshold: " + String(g_socChangeThreshold));
  Serial.println("     Warmup Delay: " + String(g_warmupDelay) + "ms");
  Serial.println("     Max Power Reading: " + String(g_maxPowerReading, 1) + "W");
}

void saveAdvancedSettingsToSPIFFS(const AdvancedSettings& settings) {
  Serial.println("[ADV] Saving advanced settings to SPIFFS...");
  
  DynamicJsonDocument doc(512);
  doc["powerThreshold"] = settings.powerThreshold;
  doc["powerFilterAlpha"] = settings.powerFilterAlpha;
  doc["voltageMinSafe"] = settings.voltageMinSafe;
  doc["batteryLowWarning"] = settings.batteryLowWarning;
  doc["batteryCritical"] = settings.batteryCritical;
  doc["autoPowerOnDelay"] = settings.autoPowerOnDelay;
  doc["socBufferSize"] = settings.socBufferSize;
  doc["socChangeThreshold"] = settings.socChangeThreshold;
  doc["powerStationOffVoltage"] = settings.powerStationOffVoltage;
  doc["warmupDelay"] = settings.warmupDelay;
  doc["maxPowerReading"] = settings.maxPowerReading;

  File configFile = SPIFFS.open(ADVANCED_SETTINGS_FILE, "w");
  if (!configFile) {
    LOG_ERROR("Advanced settings: Failed to open file for writing");
    return;
  }

  serializeJson(doc, configFile);
  configFile.close();

  // Aggiorna le variabili globali - CRITICAL SECTION
  // Disable interrupts briefly to ensure atomic updates
  noInterrupts();
  g_powerThreshold = settings.powerThreshold;
  g_powerFilterAlpha = settings.powerFilterAlpha;
  g_voltageMinSafe = settings.voltageMinSafe;
  g_batteryLowWarning = settings.batteryLowWarning;
  g_batteryCritical = settings.batteryCritical;
  g_autoPowerOnDelay = settings.autoPowerOnDelay;
  g_socBufferSize = settings.socBufferSize;
  g_socChangeThreshold = settings.socChangeThreshold;
  g_powerStationOffVoltage = settings.powerStationOffVoltage;
  g_warmupDelay = settings.warmupDelay;
  g_maxPowerReading = settings.maxPowerReading;
  interrupts();

  Serial.println("[ADV] Advanced settings saved successfully:");
  Serial.println("     Power Threshold: " + String(g_powerThreshold, 2) + "W");
  Serial.println("     Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 2) + "V");
  Serial.println("     Auto Power On Delay: " + String(g_autoPowerOnDelay) + "ms");
  Serial.println("     Warmup Delay: " + String(g_warmupDelay) + "ms");
  Serial.println("     Max Power Reading: " + String(g_maxPowerReading, 1) + "W");
}

// ===================================================================
// API PASSWORD FUNCTIONS
// ===================================================================

void loadAPIPasswordFromSPIFFS() {
  Serial.println("[API] Loading API password from SPIFFS...");
  
  if (!SPIFFS.exists(API_PASSWORD_FILE)) {
    Serial.println("[API] No API password file found, using default");
    g_apiPassword = API_PASSWORD_DEFAULT;
    return;
  }

  File file = SPIFFS.open(API_PASSWORD_FILE, "r");
  if (!file) {
    LOG_ERROR("API: Failed to open password file");
    g_apiPassword = API_PASSWORD_DEFAULT;
    return;
  }

  g_apiPassword = file.readStringUntil('\n');
  file.close();
  
  g_apiPassword.trim();
  
  if (g_apiPassword.length() == 0) {
    g_apiPassword = API_PASSWORD_DEFAULT;
  }

  Serial.println("[API] API password loaded from SPIFFS");
}

void saveAPIPasswordToSPIFFS(const String& password) {
  Serial.println("[API] Saving API password to SPIFFS...");
  
  File file = SPIFFS.open(API_PASSWORD_FILE, "w");
  if (!file) {
    LOG_ERROR("API: Failed to save password");
    return;
  }

  file.println(password);
  file.close();
  
  g_apiPassword = password;

  Serial.println("[API] API password saved successfully");
}

// ===================================================================
// SYSTEM SETTINGS FUNCTIONS (NEW!)
// ===================================================================

void loadSystemSettingsFromSPIFFS() {
  Serial.println("[SYS] Loading system settings from SPIFFS...");
  
  if (!SPIFFS.exists(SYSTEM_SETTINGS_FILE)) {
    Serial.println("[SYS] No system settings file found, using defaults");
    Serial.println("[SYS] Default values:");
    Serial.println("     NTP Server: " + g_ntpServer);
    Serial.println("     GMT Offset: " + String(g_gmtOffset) + "s");
    Serial.println("     Daylight Offset: " + String(g_daylightOffset) + "s");
    Serial.println("     Beeps Enabled: YES");
    Serial.println("     Log Level: INFO");
    return;
  }

  File configFile = SPIFFS.open(SYSTEM_SETTINGS_FILE, "r");
  if (!configFile) {
    LOG_ERROR("System settings: Failed to open file for reading");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    LOG_ERROR("System settings: Failed to parse JSON: " + String(error.c_str()));
    return;
  }

  g_ntpServer = doc["ntpServer"] | NTP_SERVER_DEFAULT;
  g_gmtOffset = doc["gmtOffset"] | NTP_GMT_OFFSET_DEFAULT;
  g_daylightOffset = doc["daylightOffset"] | NTP_DAYLIGHT_OFFSET_DEFAULT;
  g_beepsEnabled = doc["beepsEnabled"] | true;
  g_logLevel = doc["logLevel"] | LOG_LEVEL_DEFAULT;

  Serial.println("[SYS] System settings loaded from SPIFFS:");
  Serial.println("     NTP Server: " + g_ntpServer);
  Serial.println("     GMT Offset: " + String(g_gmtOffset) + "s");
  Serial.println("     Daylight Offset: " + String(g_daylightOffset) + "s");
  Serial.println("     Beeps Enabled: " + String(g_beepsEnabled ? "YES" : "NO"));
  
  String logLevelStr = "INFO";
  if (g_logLevel == LOG_LEVEL_DEBUG) logLevelStr = "DEBUG";
  else if (g_logLevel == LOG_LEVEL_WARNING) logLevelStr = "WARNING";
  else if (g_logLevel == LOG_LEVEL_ERROR) logLevelStr = "ERROR";
  else if (g_logLevel == LOG_LEVEL_NONE) logLevelStr = "NONE";
  Serial.println("     Log Level: " + logLevelStr);
}

void saveSystemSettingsToSPIFFS(const SystemSettings& settings) {
  Serial.println("[SYS] Saving system settings to SPIFFS...");
  
  DynamicJsonDocument doc(512);
  doc["ntpServer"] = settings.ntpServer;
  doc["gmtOffset"] = settings.gmtOffset;
  doc["daylightOffset"] = settings.daylightOffset;
  doc["beepsEnabled"] = settings.beepsEnabled;
  doc["logLevel"] = settings.logLevel;

  File configFile = SPIFFS.open(SYSTEM_SETTINGS_FILE, "w");
  if (!configFile) {
    LOG_ERROR("System settings: Failed to open file for writing");
    return;
  }

  serializeJson(doc, configFile);
  configFile.close();

  g_ntpServer = settings.ntpServer;
  g_gmtOffset = settings.gmtOffset;
  g_daylightOffset = settings.daylightOffset;
  g_beepsEnabled = settings.beepsEnabled;
  g_logLevel = settings.logLevel;

  Serial.println("[SYS] System settings saved successfully:");
  Serial.println("     NTP Server: " + g_ntpServer);
  Serial.println("     GMT Offset: " + String(g_gmtOffset) + "s");
  Serial.println("     Daylight Offset: " + String(g_daylightOffset) + "s");
  Serial.println("     Beeps Enabled: " + String(g_beepsEnabled ? "YES" : "NO"));
  
  String logLevelStr = "INFO";
  if (g_logLevel == LOG_LEVEL_DEBUG) logLevelStr = "DEBUG";
  else if (g_logLevel == LOG_LEVEL_WARNING) logLevelStr = "WARNING";
  else if (g_logLevel == LOG_LEVEL_ERROR) logLevelStr = "ERROR";
  else if (g_logLevel == LOG_LEVEL_NONE) logLevelStr = "NONE";
  Serial.println("     Log Level: " + logLevelStr);
}

// ===================================================================
// HTTP SHUTDOWN CONFIG FUNCTIONS (NEW!)
// ===================================================================

void loadHttpShutdownConfigFromSPIFFS() {
  Serial.println("[SHUTDOWN] Loading HTTP shutdown config from SPIFFS...");
  
  if (!SPIFFS.exists(HTTP_SHUTDOWN_CONFIG_FILE)) {
    Serial.println("[SHUTDOWN] No HTTP shutdown config file found, using defaults");
    Serial.println("[SHUTDOWN] Default values:");
    Serial.println("     Enabled: " + String(g_httpShutdownEnabled ? "YES" : "NO"));
    Serial.println("     Threshold: " + String(g_httpShutdownThreshold, 1) + "%");
    Serial.println("     Server: " + g_httpShutdownServer);
    Serial.println("     Port: " + String(g_httpShutdownPort));
    return;
  }

  File configFile = SPIFFS.open(HTTP_SHUTDOWN_CONFIG_FILE, "r");
  if (!configFile) {
    LOG_ERROR("HTTP Shutdown: Failed to open config file for reading");
    return;
  }

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    LOG_ERROR("HTTP Shutdown: Failed to parse config JSON: " + String(error.c_str()));
    return;
  }

  g_httpShutdownEnabled = doc["enabled"] | false;
  g_httpShutdownThreshold = doc["batteryThreshold"] | HTTP_SHUTDOWN_THRESHOLD_DEFAULT;
  g_httpShutdownServer = doc["server"] | HTTP_SHUTDOWN_SERVER_DEFAULT;
  g_httpShutdownPort = doc["port"] | HTTP_SHUTDOWN_PORT_DEFAULT;
  g_httpShutdownPassword = doc["password"] | HTTP_SHUTDOWN_PASSWORD_DEFAULT;
  g_httpShutdownSent = false;  // Always reset on boot

  Serial.println("[SHUTDOWN] HTTP shutdown config loaded from SPIFFS:");
  Serial.println("     Enabled: " + String(g_httpShutdownEnabled ? "YES" : "NO"));
  Serial.println("     Threshold: " + String(g_httpShutdownThreshold, 1) + "%");
  Serial.println("     Server: " + g_httpShutdownServer);
  Serial.println("     Port: " + String(g_httpShutdownPort));
}

void saveHttpShutdownConfigToSPIFFS(const HttpShutdownConfig& config) {
  Serial.println("[SHUTDOWN] Saving HTTP shutdown config to SPIFFS...");
  
  DynamicJsonDocument doc(512);
  doc["enabled"] = config.enabled;
  doc["batteryThreshold"] = config.batteryThreshold;
  doc["server"] = config.server;
  doc["port"] = config.port;
  doc["password"] = config.password;

  File configFile = SPIFFS.open(HTTP_SHUTDOWN_CONFIG_FILE, "w");
  if (!configFile) {
    LOG_ERROR("HTTP Shutdown: Failed to open config file for writing");
    return;
  }

  serializeJson(doc, configFile);
  configFile.close();

  g_httpShutdownEnabled = config.enabled;
  g_httpShutdownThreshold = config.batteryThreshold;
  g_httpShutdownServer = config.server;
  g_httpShutdownPort = config.port;
  g_httpShutdownPassword = config.password;
  g_httpShutdownSent = false;  // Reset when config changes

  Serial.println("[SHUTDOWN] HTTP shutdown config saved successfully:");
  Serial.println("     Enabled: " + String(g_httpShutdownEnabled ? "YES" : "NO"));
  Serial.println("     Threshold: " + String(g_httpShutdownThreshold, 1) + "%");
  Serial.println("     Server: " + g_httpShutdownServer);
  Serial.println("     Port: " + String(g_httpShutdownPort));
}