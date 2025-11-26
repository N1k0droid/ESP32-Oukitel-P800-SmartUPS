/*
 * Oukitel P800E Power Station Domotization Firmware
 * ESP32 WROOM-32D
 * 
 * Hardware:
 * - SCT013 100A current sensor on GPIO34 (Main IN)
 * - SCT013 100A current sensor on GPIO35 (Output)
 * - Battery voltage divider on GPIO36
 * - 5 optocouplers for button control
 * - Step-down power supply from 24V battery
 */



#include "config.h"
#include "hardware_manager.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "ups_protocol.h"
#include "mqtt_client.h"
#include "http_client.h"
#include "data_logger.h"
#include "energy_monitor.h"
#include "logger.h"



// Global objects
HardwareManager hardware;
WiFiManager wifiMgr;
WebServerManager webServer;
UPSProtocol upsProtocol;
MQTTClientManager mqttClient;
HTTPClientManager httpClient;
DataLogger dataLogger;
PowerStationMonitor energyMonitor;



// Timing variables
unsigned long lastSensorUpdate = 0;
unsigned long lastDataLog = 0;
unsigned long lastHealthCheck = 0;
unsigned long lastMqttPublish = 0;
unsigned long lastHttpPublish = 0;
unsigned long lastSerialLog = 0;
unsigned long lastSerialHeader = 0;
unsigned long lastStateCheck = 0;
unsigned long lastEmergencyCheck = 0;
unsigned long lastBeepUpdate = 0;
bool headerPrinted = false;



void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Configure watchdog timer for automatic recovery
  // ESP32 has built-in watchdog, but we can configure it explicitly
  // Default timeout is ~8 seconds, which is usually sufficient
  // If you experience frequent resets, increase timeout or disable
  // Note: Watchdog is automatically enabled on ESP32
  
  Serial.println("\n╔═════════════════════════════════════════════════╗");
  Serial.println("║   OUKITEL P800E DOMOTIZATION SYSTEM            ║");
  Serial.println("║   ESP32 WROOM-32D Firmware v1.1.0              ║");
  Serial.println("╚═════════════════════════════════════════════════╝\n");
  
  // Initialize SPIFFS for persistent storage
  LOG_INFO("Initializing SPIFFS...");
  if (!SPIFFS.begin(true)) {
    LOG_ERROR("SPIFFS initialization failed!");
  } else {
    LOG_INFO("SPIFFS initialized successfully");
  }
  
  // Load calibration from SPIFFS
  Serial.println("[INIT] Loading calibration data from SPIFFS...");
  loadCalibrationFromSPIFFS();
  Serial.println("[INIT] Calibration data loaded");
  Serial.println("      SCT013 Cal In: " + String(g_sct013CalIn, 2));
  Serial.println("      Voltage Offset Rest: " + String(g_voltageOffsetRest, 2));
  
  // Load advanced settings from SPIFFS
  Serial.println("[INIT] Loading advanced settings from SPIFFS...");
  loadAdvancedSettingsFromSPIFFS();
  Serial.println("[INIT] Advanced settings loaded");
  Serial.println("      Power Threshold: " + String(g_powerThreshold, 2) + "W");
  Serial.println("      Power Station OFF Voltage: " + String(g_powerStationOffVoltage, 1) + "V");
  Serial.println("      Auto Power On Delay: " + String(g_autoPowerOnDelay) + "ms");
  Serial.println("      Warmup Delay: " + String(g_warmupDelay) + "ms");
  
  // Load API password from SPIFFS
  Serial.println("[INIT] Loading API password from SPIFFS...");
  loadAPIPasswordFromSPIFFS();
  Serial.println("[INIT] API password loaded");
  
  // Load system settings from SPIFFS (NEW!)
  Serial.println("[INIT] Loading system settings from SPIFFS...");
  loadSystemSettingsFromSPIFFS();
  Serial.println("[INIT] System settings loaded");
  Serial.println("      NTP Server: " + g_ntpServer);
  Serial.println("      GMT Offset: " + String(g_gmtOffset) + "s");
  Serial.println("      Daylight Offset: " + String(g_daylightOffset) + "s");
  
  // Load HTTP shutdown config from SPIFFS (NEW!)
  Serial.println("[INIT] Loading HTTP shutdown config from SPIFFS...");
  loadHttpShutdownConfigFromSPIFFS();
  Serial.println("[INIT] HTTP shutdown config loaded");
  Serial.println("      Enabled: " + String(g_httpShutdownEnabled ? "YES" : "NO"));
  Serial.println("      Threshold: " + String(g_httpShutdownThreshold, 1) + "%");
  Serial.println("      Server: " + g_httpShutdownServer);
  
  // Initialize hardware
  LOG_INFO("Initializing hardware...");
  if (!hardware.begin()) {
    LOG_ERROR("Hardware initialization failed!");
    ESP.restart();
  }
  
  // Initialize data storage
  Serial.println("[INIT] Initializing data storage...");
  if (!dataLogger.begin()) {
    Serial.println("[ERROR] Data storage initialization failed!");
  }
  
  // Initialize energy monitoring
  Serial.println("[INIT] Initializing energy monitor...");
  energyMonitor.begin();
  
  // Initialize WiFi
  Serial.println("[INIT] Initializing WiFi...");
  wifiMgr.begin();
  
  // Initialize NTP time synchronization with configured server (UPDATED!)
  Serial.println("[INIT] Initializing NTP time sync...");
  Serial.println("[INIT] Using NTP server: " + g_ntpServer);
  configTime(g_gmtOffset, g_daylightOffset, g_ntpServer.c_str());
  Serial.println("[INIT] NTP configured. Waiting for time sync...");
  
  // Wait for NTP sync (max 10 seconds)
  int ntpRetries = 0;
  while (time(nullptr) < 1000000000 && ntpRetries < 20) {
    delay(500);
    Serial.print(".");
    ntpRetries++;
  }
  Serial.println();
  
  if (time(nullptr) >= 1000000000) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    Serial.println("[INIT] NTP time synchronized: " + String(timeStr));
    
    // Synchronize energy monitor with NTP time
    energyMonitor.syncTimeAfterNTP();
    Serial.println("[INIT] Energy monitor time synchronized");
  } else {
    Serial.println("[INIT] NTP sync timeout - will retry in background");
  }
  
  // Initialize web server
  Serial.println("[INIT] Initializing web server...");
  webServer.begin();
  
  // Set WebServer reference in HardwareManager
  hardware.setWebServerReference(&webServer);
    
  // Initialize UPS protocol
  Serial.println("[INIT] Initializing UPS protocol...");
  upsProtocol.begin();
  
  // Initialize MQTT client
  Serial.println("[INIT] Initializing MQTT client...");
  mqttClient.begin();
  
  // Update MQTT client ID with real MAC address
  if (wifiMgr.isConnected()) {
    Serial.println("[INIT] Updating MQTT client ID with real MAC address...");
    mqttClient.updateClientIdWithMAC();
  }
  
  // Initialize HTTP client
  Serial.println("[INIT] Initializing HTTP client...");
  httpClient.begin();
  
  Serial.println("\n[INIT] System initialization complete!");
  
  // Print connection info
  if (wifiMgr.isConnected()) {
    Serial.println("WiFi: Connected to " + wifiMgr.getSSID());
    Serial.println("Web interface: http://" + wifiMgr.getLocalIP());
    Serial.println("IP Address: " + wifiMgr.getLocalIP());
  } else if (wifiMgr.isAPMode()) {
    Serial.println("WiFi: AP Mode - " + String(AP_SSID));
    Serial.println("Web interface: http://" + wifiMgr.getAPIP());
  }
  
  if (upsProtocol.isEnabled()) {
    Serial.println("UPS protocol: Listening on port " + String(upsProtocol.getConfig().port));
  }
  
  if (mqttClient.isConnected()) {
    Serial.println("MQTT: Connected to broker");
  }
  
  if (httpClient.isEnabled()) {
    Serial.println("HTTP: Home Assistant integration enabled");
  }
  
  if (g_httpShutdownEnabled) {
    Serial.println("HTTP Shutdown: Enabled (Threshold: " + String(g_httpShutdownThreshold, 1) + "%)");
  }
  
  Serial.println();
}



void loop() {
  // Update WiFi connection
  static bool wasConnected = false;
  bool isConnected = wifiMgr.isConnected();
  
  // Check if WiFi just connected (transition from disconnected to connected)
  if (isConnected && !wasConnected) {
    LOG_INFO("WiFi just connected - updating MQTT client ID with MAC address");
    mqttClient.updateClientIdWithMAC();
  }
  wasConnected = isConnected;
  
  wifiMgr.handleConnection();
  
  // Handle web server and websocket
  webServer.handleClient();
  
  // Handle UPS protocol
  upsProtocol.handleClients();
  
  // Handle MQTT client (only if WiFi is connected)
  if (isConnected) {
    mqttClient.loop();
    httpClient.loop();
  }
  
  // Update sensor readings every second
  if (timeElapsed(lastSensorUpdate, 1000)) {
    hardware.readSensors();
    SensorData data = hardware.getSensorData();
    
    // Update energy monitor only after warm-up is complete
    if (hardware.getIsWarmedUp()) {
      energyMonitor.update(data);
    }
    
    // Print header after first valid reading
    if (!headerPrinted && data.batteryVoltage > 0) {
      hardware.printStatusHeader();
      headerPrinted = true;
      lastSerialHeader = millis();
    }
    
    // Update UPS status only after warm-up
    if (data.batteryVoltage > 0) {
      upsProtocol.updateStatus(data);
    }
    
    lastSensorUpdate = millis();
  }
  
  // Check state transitions every second
  if (timeElapsed(lastStateCheck, 1000)) {
    hardware.checkStateTransition();
    lastStateCheck = millis();
  }
  
  // Check Auto Power On
  hardware.checkAutoPowerOn();
  
  // Check emergency conditions every second
  if (timeElapsed(lastEmergencyCheck, 1000)) {
    hardware.checkEmergencyConditions();
    lastEmergencyCheck = millis();
  }
  
  // Update beep state continuously (non-blocking)
  hardware.updateBeepState();
  
  // Update button state continuously (non-blocking)
  hardware.updateButtonState();
  
  // Log data every 5 minutes
  if (timeElapsed(lastDataLog, 300000)) {
    SensorData data = hardware.getSensorData();
    EnergyData energyData = energyMonitor.getEnergyData();
    dataLogger.logData(data, energyData);
    lastDataLog = millis();
  }
  
  // Publish MQTT data every 30 seconds (if connected)
  if (wifiMgr.isConnected() && mqttClient.isConnected()) {
    if (timeElapsed(lastMqttPublish, 30000)) {
      SensorData data = hardware.getSensorData();
      EnergyData energyData = energyMonitor.getEnergyData();
      mqttClient.publishData(data, energyData);
      lastMqttPublish = millis();
    }
  }
  
  // Publish HTTP data every 30 seconds (if enabled)
  if (wifiMgr.isConnected() && httpClient.isEnabled()) {
    if (timeElapsed(lastHttpPublish, 30000)) {
      SensorData data = hardware.getSensorData();
      EnergyData energyData = energyMonitor.getEnergyData();
      httpClient.publishData(data, energyData);
      lastHttpPublish = millis();
    }
  }
  
  // System health check every 30 seconds
  if (timeElapsed(lastHealthCheck, 30000)) {
    SensorData data = hardware.getSensorData();
    if (data.batteryVoltage > 0) {
      checkSystemHealth();
    }
    
    // Log memory status for debugging
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    if (freeHeap < 15000) {
      LOG_WARNING("Low free heap: " + String(freeHeap) + " bytes (min: " + String(minFreeHeap) + ")");
    }
    
    lastHealthCheck = millis();
  }
  
  // Formatted serial log every 30 seconds (only after warm-up)
  if (headerPrinted && timeElapsed(lastSerialLog, SERIAL_LOG_INTERVAL)) {
    hardware.printStatusLine();
    lastSerialLog = millis();
  }
  
  // Print header every 5 minutes
  if (headerPrinted && timeElapsed(lastSerialHeader, SERIAL_LOG_HEADER_INTERVAL)) {
    hardware.printStatusHeader();
    lastSerialHeader = millis();
  }
  
  // Prevent watchdog timeout and allow other tasks to run
  yield();
  
  // Small delay to help with timing and watchdog
  delay(1);
}

// Helper function to safely check if time has elapsed (handles millis() overflow)
bool timeElapsed(unsigned long startTime, unsigned long interval) {
  return (millis() - startTime) >= interval;
}



void checkSystemHealth() {
  // Check for low memory
  if (ESP.getFreeHeap() < 10000) {
    LOG_WARNING("Low memory: " + String(ESP.getFreeHeap()) + " bytes");
  }
  
  // Check sensor health
  SensorData data = hardware.getSensorData();
  
  // Emergency conditions are handled in checkEmergencyConditions()
  
  // Publish data during health check if connected
  if (wifiMgr.isConnected()) {
    EnergyData energyData = energyMonitor.getEnergyData();
    
    if (mqttClient.isConnected()) {
      mqttClient.publishData(data, energyData);
    }
    
    if (httpClient.isEnabled()) {
      httpClient.publishData(data, energyData);
    }
  }
}
