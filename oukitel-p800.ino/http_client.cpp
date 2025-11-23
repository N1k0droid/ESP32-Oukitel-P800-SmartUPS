/*
 * HTTP Client Manager Implementation
 */

#include "http_client.h"
#include "hardware_manager.h"
#include "logger.h"
#include <SPIFFS.h>

// External references
extern HardwareManager hardware;

HTTPClientManager::HTTPClientManager() {
  initialized = false;
  lastPublish = 0;
  shutdownNotificationSent = false;
  
  // Initialize config with defaults
  config.server = "";
  config.port = 8123;
  config.endpoint = "/api/states/sensor.oukitel_p800e";
  config.apiKey = "";
  config.enabled = false;
}

bool HTTPClientManager::begin() {
  Serial.println("[HTTP] Initializing HTTP client...");
  
  loadConfig();
  
  if (!config.enabled || config.server.length() == 0) {
    Serial.println("[HTTP] HTTP client disabled or not configured");
    return true;
  }
  
  Serial.println("[HTTP] HTTP client configured");
  Serial.println("  Server: " + config.server + ":" + String(config.port));
  Serial.println("  Endpoint: " + config.endpoint);
  
  initialized = true;
  return true;
}

void HTTPClientManager::loop() {
  if (!initialized || !config.enabled) return;
  
  // Auto-publish every 30 seconds
  if (millis() - lastPublish >= 30000) {
    // This will be called from main loop with actual data
    lastPublish = millis();
  }
}

void HTTPClientManager::publishData(const SensorData& sensorData, const EnergyData& energyData) {
  if (!initialized || !config.enabled) return;
  
  // Check battery shutdown threshold
  checkBatteryShutdownThreshold(sensorData);
  
  // Create JSON payload - increased size for safety
  DynamicJsonDocument doc(1536);
  doc["state"] = sensorData.batteryPercentage;
  
  JsonObject attributes = doc.createNestedObject("attributes");
  attributes["voltage"] = sensorData.batteryVoltage;
  attributes["main_current"] = sensorData.mainCurrent;
  attributes["output_current"] = sensorData.outputCurrent;
  attributes["main_power"] = sensorData.mainPower;
  attributes["output_power"] = sensorData.outputPower;
  attributes["battery_percentage"] = sensorData.batteryPercentage;
  attributes["on_battery"] = sensorData.onBattery;
  attributes["battery_state"] = sensorData.batteryState;
  attributes["instant_power"] = energyData.instantPower;
  attributes["daily_consumption"] = energyData.dailyConsumption;
  attributes["monthly_consumption"] = energyData.monthlyConsumption;
  attributes["peak_power"] = energyData.peakPower;
  attributes["operating_time"] = energyData.operatingTime;
  attributes["timestamp"] = sensorData.timestamp;
  
  String payload;
  size_t bytesWritten = serializeJson(doc, payload);
  
  if (bytesWritten == 0 || payload.length() == 0) {
    Serial.println("[HTTP] ERROR: Failed to serialize data JSON");
    return;
  }
  
  // Send to Home Assistant
  String url = "http://" + config.server + ":" + String(config.port) + config.endpoint;
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + config.apiKey);
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      Serial.println("[HTTP] Data published successfully");
    } else {
      Serial.println("[HTTP] Publish failed with code: " + String(httpCode));
    }
  } else {
    Serial.println("[HTTP] Connection failed: " + http.errorToString(httpCode));
  }
  
  http.end();
}

void HTTPClientManager::checkBatteryShutdownThreshold(const SensorData& sensorData) {
  // Check if HTTP shutdown is enabled
  if (!g_httpShutdownEnabled) {
    return;
  }
  
  // Check if server is configured
  if (g_httpShutdownServer.length() == 0) {
    return;
  }
  
  // Check if notification was already sent
  if (g_httpShutdownSent) {
    // Reset flag if battery is above threshold + 5% hysteresis
    if (sensorData.batteryPercentage > (g_httpShutdownThreshold + 5.0)) {
      g_httpShutdownSent = false;
      Serial.println("[SHUTDOWN] Battery recovered, reset shutdown notification flag");
    }
    return;
  }
  
  // Check if battery is below threshold
  if (sensorData.batteryPercentage <= g_httpShutdownThreshold) {
    Serial.println("[SHUTDOWN] Battery below threshold (" + String(sensorData.batteryPercentage, 1) + "% <= " + String(g_httpShutdownThreshold, 1) + "%)");
    
    if (sendShutdownNotification()) {
      g_httpShutdownSent = true;
      Serial.println("[SHUTDOWN] Shutdown notification sent successfully");
    } else {
      Serial.println("[SHUTDOWN] Failed to send shutdown notification");
    }
  }
}

bool HTTPClientManager::sendShutdownNotification() {
  // Create JSON payload with password
  DynamicJsonDocument doc(512);
  doc["event"] = "battery_shutdown";
  doc["battery_percentage"] = hardware.getSensorData().batteryPercentage;
  doc["battery_voltage"] = hardware.getSensorData().batteryVoltage;
  doc["password"] = g_httpShutdownPassword;
  doc["timestamp"] = millis();
  doc["device"] = DEVICE_NAME;
  
  String payload;
  serializeJson(doc, payload);
  
  // Send to configured shutdown server
  String url = "http://" + g_httpShutdownServer + ":" + String(g_httpShutdownPort) + "/shutdown";
  
  Serial.println("[SHUTDOWN] Sending shutdown notification to: " + url);
  Serial.println("[SHUTDOWN] Payload: " + payload);
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);  // 5 second timeout
  
  int httpCode = http.POST(payload);
  
  bool success = false;
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_ACCEPTED) {
      Serial.println("[SHUTDOWN] Shutdown notification accepted (HTTP " + String(httpCode) + ")");
      String response = http.getString();
      Serial.println("[SHUTDOWN] Response: " + response);
      success = true;
    } else {
      Serial.println("[SHUTDOWN] Shutdown notification failed with HTTP code: " + String(httpCode));
    }
  } else {
    Serial.println("[SHUTDOWN] Connection failed: " + http.errorToString(httpCode));
  }
  
  http.end();
  return success;
}

bool HTTPClientManager::executeCommand(const String& command, const String& value) {
  if (!initialized || !config.enabled) return false;
  
  Serial.println("[HTTP] Executing command: " + command + " = " + value);
  
  // Map commands to button presses
  if (command == "usb") {
    hardware.pressButton(BTN_USB, BUTTON_STANDARD_DURATION);
    return true;
  } else if (command == "dc") {
    hardware.pressButton(BTN_DC, BUTTON_STANDARD_DURATION);
    return true;
  } else if (command == "ac") {
    hardware.pressButton(BTN_AC, BUTTON_STANDARD_DURATION);
    return true;
  } else if (command == "flashlight") {
    hardware.pressButton(BTN_FLASHLIGHT, BUTTON_STANDARD_DURATION);
    return true;
  } else if (command == "power") {
    hardware.pressButton(BTN_POWER, BUTTON_POWER_DURATION);
    return true;
  }
  
  return false;
}

bool HTTPClientManager::isEnabled() {
  return initialized && config.enabled;
}

bool HTTPClientManager::setConfig(const HTTPConfig& newConfig) {
  config = newConfig;
  saveConfig();
  
  if (config.enabled && config.server.length() > 0) {
    initialized = true;
  } else {
    initialized = false;
  }
  
  return true;
}

HTTPConfig HTTPClientManager::getConfig() {
  return config;
}

void HTTPClientManager::loadConfig() {
  if (!SPIFFS.exists("/ha_config.json")) {
    Serial.println("[HTTP] No saved HTTP configuration found");
    return;
  }
  
  File file = SPIFFS.open("/ha_config.json", "r");
  if (!file) {
    Serial.println("[HTTP] Failed to open HTTP config file");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("[HTTP] Failed to parse HTTP config file");
    return;
  }
  
  config.server = doc["server"].as<String>();
  config.port = doc["port"] | 8123;
  config.endpoint = doc["endpoint"].as<String>();
  config.apiKey = doc["apiKey"].as<String>();
  config.enabled = doc["enabled"] | false;
  
  if (config.endpoint.length() == 0) {
    config.endpoint = "/api/states/sensor.oukitel_p800e";
  }
  
  Serial.println("[HTTP] Loaded HTTP configuration");
}

void HTTPClientManager::saveConfig() {
  DynamicJsonDocument doc(1024);
  doc["server"] = config.server;
  doc["port"] = config.port;
  doc["endpoint"] = config.endpoint;
  doc["apiKey"] = config.apiKey;
  doc["enabled"] = config.enabled;
  
  File file = SPIFFS.open("/ha_config.json", "w");
  if (!file) {
    Serial.println("[HTTP] Failed to save HTTP configuration");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("[HTTP] HTTP configuration saved");
}