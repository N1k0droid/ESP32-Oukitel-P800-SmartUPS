/*
 * Web Server Manager Implementation - Core Functions
 */

#include "web_server.h"
#include "hardware_manager.h"
#include "wifi_manager.h"
#include "data_logger.h"
#include "energy_monitor.h"
#include "config.h"
#include "logger.h"
#include <SPIFFS.h>

// Static instance pointer
WebServerManager* WebServerManager::instance = nullptr;

// External references
extern HardwareManager hardware;
extern WiFiManager wifiMgr;
extern DataLogger dataLogger;
extern PowerStationMonitor energyMonitor;
extern String g_apiPassword;

WebServerManager::WebServerManager() : server(WEB_SERVER_PORT), webSocket(WEBSOCKET_PORT) {
  initialized = false;
  lastDataBroadcast = 0;
  instance = this;
}

bool WebServerManager::begin() {
  Serial.println("[WEB] Initializing web server...");

  // Setup routes
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/api/data", HTTP_GET, [this]() { handleAPI(); });
  server.on("/api/command", HTTP_POST, [this]() { handleAPICommand(); });
  server.on("/api/config", HTTP_GET, [this]() { handleConfig(); });
  server.on("/api/config", HTTP_POST, [this]() { handleConfig(); });
  server.on("/api/wifi", HTTP_GET, [this]() { handleWiFiConfig(); });
  server.on("/api/wifi", HTTP_POST, [this]() { handleWiFiConfig(); });
  server.on("/api/button", HTTP_POST, [this]() { handleButtonPress(); });
  server.onNotFound([this]() { handleNotFound(); });

  // WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEventStatic);

  server.begin();

  Serial.println("[WEB] Web server started on port " + String(WEB_SERVER_PORT));
  Serial.println("[WEB] WebSocket server started on port " + String(WEBSOCKET_PORT));
  Serial.println("[WEB] HTTP API endpoint: /api/command (password protected)");

  initialized = true;
  return true;
}

// Helper function to safely check if time has elapsed (handles millis() overflow)
static bool webTimeElapsed(unsigned long startTime, unsigned long interval) {
  return (millis() - startTime) >= interval;
}

void WebServerManager::handleClient() {
  if (!initialized) return;

  // Check memory before processing - prevent crashes
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < 5000) {
    // Critical memory low - skip non-essential operations
    static unsigned long lastMemoryWarning = 0;
    if (webTimeElapsed(lastMemoryWarning, 60000)) {  // Warn every minute
      LOG_ERROR("Web server: Low memory (" + String(freeHeap) + " bytes) - skipping broadcasts");
      lastMemoryWarning = millis();
    }
    // Still handle client requests but skip broadcasts
    server.handleClient();
    webSocket.loop();
    return;
  }

  server.handleClient();
  webSocket.loop();

  // Broadcast every 5 seconds
  if (webTimeElapsed(lastDataBroadcast, 5000)) {
    // Check memory before broadcast
    if (ESP.getFreeHeap() > 10000) {
      SensorData sensorData = hardware.getSensorData();
      EnergyData energyData = energyMonitor.getStableEnergyData();
      
      broadcastData(sensorData, energyData);
      lastDataBroadcast = millis();
    } else {
      // Skip broadcast if memory is low
      LOG_WARNING("Web server: Low memory, skipping WebSocket broadcast");
    }
  }
}

void WebServerManager::handleRoot() {
  // Protect web interface with Basic Auth
  if (!server.authenticate(WEB_USERNAME, WEB_PASSWORD)) {
    return server.requestAuthentication();
  }
  
  // Check memory before generating large HTML
  if (ESP.getFreeHeap() < 20000) {
    LOG_WARNING("Web server: Low memory, sending simplified response");
    server.send(200, "text/html", "<html><body><h1>Oukitel P800E</h1><p>System is running but memory is low. Please refresh.</p></body></html>");
    return;
  }
  
  sendCORS();
  
  // Generate HTML with error handling
  String html = generateHTML();
  if (html.length() == 0) {
    Serial.println("[WEB] ERROR: Failed to generate HTML");
    server.send(500, "text/html", "<html><body><h1>Error</h1><p>Failed to generate page. Please try again.</p></body></html>");
    return;
  }
  
  server.send(200, "text/html", html);
}

void WebServerManager::handleAPI() {
  sendCORS();

  SensorData data = hardware.getSensorData();

  // Increased buffer size for safety
  DynamicJsonDocument doc(2048);
  doc["timestamp"] = data.timestamp;
  doc["mainCurrent"] = data.mainCurrent;
  doc["outputCurrent"] = data.outputCurrent;
  doc["batteryVoltage"] = data.batteryVoltage;
  doc["batteryPercentage"] = data.batteryPercentage;
  doc["mainPower"] = data.mainPower;
  doc["outputPower"] = data.outputPower;
  doc["onBattery"] = data.onBattery;

  doc["wifiStatus"] = wifiMgr.getConnectionStatus();
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["autoPowerOn"] = hardware.getAutoPowerOn();

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

bool WebServerManager::validateAPIPassword(const String& password) {
  return password == g_apiPassword;
}

void WebServerManager::handleAPICommand() {
  sendCORS();
  
  // Check for password in header
  String password = "";
  if (server.hasHeader("X-API-Password")) {
    password = server.header("X-API-Password");
  } else if (server.hasArg("password")) {
    password = server.arg("password");
  }
  
  if (!validateAPIPassword(password)) {
    Serial.println("[API] Unauthorized API command attempt");
    server.send(401, "application/json", "{\"error\":\"Unauthorized - Invalid password\"}");
    return;
  }
  
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Missing request body\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Validate body size to prevent buffer overflow
  if (body.length() == 0 || body.length() > 2048) {
    server.send(400, "application/json", "{\"error\":\"Invalid request body size\"}");
    return;
  }
  
  // Increased buffer size for command parsing
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    Serial.println("[API] JSON parse error: " + String(error.c_str()));
    server.send(400, "application/json", "{\"error\":\"Invalid JSON: " + String(error.c_str()) + "\"}");
    return;
  }
  
  // Validate command parameter exists
  if (!doc.containsKey("command")) {
    server.send(400, "application/json", "{\"error\":\"Missing 'command' parameter\"}");
    return;
  }
  
  String command = doc["command"].as<String>();
  
  // Validate command string length
  if (command.length() == 0 || command.length() > 64) {
    server.send(400, "application/json", "{\"error\":\"Invalid command string\"}");
    return;
  }
  
  if (command == "pressButton") {
    // Validate button index
    if (!doc.containsKey("button")) {
      server.send(400, "application/json", "{\"error\":\"Missing 'button' parameter\"}");
      return;
    }
    
    int button = doc["button"].as<int>();
    if (button < 0 || button > 4) {
      server.send(400, "application/json", "{\"error\":\"Invalid button index (must be 0-4)\"}");
      return;
    }
    
    // Optional: validate duration if provided
    int duration = -1;
    if (doc.containsKey("duration")) {
      duration = doc["duration"].as<int>();
      if (duration < 100 || duration > 10000) {
        server.send(400, "application/json", "{\"error\":\"Invalid duration (100-10000ms)\"}");
        return;
      }
    } else {
      duration = (button == 0) ? BUTTON_POWER_DURATION : BUTTON_STANDARD_DURATION;
    }
    
    hardware.pressButton(button, duration);
    
    Serial.println("[API] Button " + String(button) + " pressed via HTTP API");
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Button pressed\"}");
    
  } else if (command == "setAutoPowerOn") {
    // Validate enabled parameter
    if (!doc.containsKey("enabled")) {
      server.send(400, "application/json", "{\"error\":\"Missing 'enabled' parameter\"}");
      return;
    }
    
    bool enabled = doc["enabled"].as<bool>();
    hardware.setAutoPowerOn(enabled);
    
    Serial.println("[API] Auto Power On set to " + String(enabled ? "ENABLED" : "DISABLED") + " via HTTP API");
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Auto Power On updated\"}");
    
  } else if (command == "getData") {
    SensorData data = hardware.getSensorData();
    EnergyData energyData = energyMonitor.getStableEnergyData();
    
    // Increased buffer size for response
    DynamicJsonDocument response(2048);
    response["success"] = true;
    response["voltage"] = data.batteryVoltage;
    response["soc"] = data.batteryPercentage;
    response["powerIn"] = data.mainPower;
    response["powerOut"] = data.outputPower;
    response["state"] = hardware.getStateString(data.batteryState);
    response["instantPower"] = energyData.instantPower;
    response["dailyConsumption"] = energyData.dailyConsumption;
    response["monthlyConsumption"] = energyData.monthlyConsumption;
    
    String output;
    serializeJson(response, output);
    server.send(200, "application/json", output);
    
  } else {
    server.send(400, "application/json", "{\"error\":\"Unknown command\"}");
  }
}

void WebServerManager::handleConfig() {
  // Protect configuration page
  if (!server.authenticate(WEB_USERNAME, WEB_PASSWORD)) {
    return server.requestAuthentication();
  }
  sendCORS();

  if (server.method() == HTTP_GET) {
    server.send(200, "text/html", generateConfigHTML());
  } else if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    // Increased buffer size
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }

    // Configuration is handled via WebSocket API
    server.send(200, "application/json", "{\"success\":true}");
  }
}

void WebServerManager::handleWiFiConfig() {
  // Protect WiFi configuration page
  if (!server.authenticate(WEB_USERNAME, WEB_PASSWORD)) {
    return server.requestAuthentication();
  }
  sendCORS();

  if (server.method() == HTTP_GET) {
    server.send(200, "text/html", generateWiFiConfigHTML());
  } else if (server.method() == HTTP_POST) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if (ssid.length() > 0) {
      if (wifiMgr.setCredentials(ssid, password)) {
        wifiMgr.saveCredentials();
        server.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi credentials updated\"}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid credentials\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"SSID required\"}");
    }
  }
}

void WebServerManager::handleButtonPress() {
  sendCORS();

  if (!server.hasArg("button")) {
    server.send(400, "application/json", "{\"error\":\"Button parameter required\"}");
    return;
  }

  int buttonIndex = server.arg("button").toInt();
  int duration = BUTTON_STANDARD_DURATION;

  if (buttonIndex == 0) {
    duration = BUTTON_POWER_DURATION;
  }

  if (hardware.pressButton(buttonIndex, duration)) {
    server.send(200, "application/json", "{\"success\":true}");

    DynamicJsonDocument doc(256);
    doc["type"] = "buttonPress";
    doc["button"] = buttonIndex;
    doc["duration"] = duration;

    String message;
    serializeJson(doc, message);
    webSocket.broadcastTXT(message);
  } else {
    server.send(400, "application/json", "{\"error\":\"Button press failed\"}");
  }
}

void WebServerManager::handleNotFound() {
  sendCORS();
  server.send(404, "text/plain", "Not Found");
}

void WebServerManager::sendCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, X-API-Password");
}

void WebServerManager::webSocketEventStatic(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (instance) {
    instance->webSocketEvent(num, type, payload, length);
  }
}

void WebServerManager::broadcastData(const SensorData& sensorData, const EnergyData& energyData) {
  // Check memory before creating JSON
  if (ESP.getFreeHeap() < 10000) {
    return;  // Skip broadcast if memory too low
  }
  
  // Increased buffer size
  DynamicJsonDocument doc(2048);
  doc["type"] = "sensorData";
  doc["timestamp"] = sensorData.timestamp;
  doc["voltage"] = sensorData.batteryVoltage;
  doc["soc"] = sensorData.batteryPercentage;
  doc["powerIn"] = sensorData.mainPower;
  doc["powerOut"] = sensorData.outputPower;
  doc["state"] = hardware.getStateString(sensorData.batteryState);
  doc["autoPowerOn"] = hardware.getAutoPowerOn();
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;

  // Energy data from energyMonitor
  doc["instantPower"] = energyData.instantPower;
  doc["dailyConsumption"] = energyData.dailyConsumption;
  doc["monthCurrent"] = energyData.monthlyConsumption;
  doc["yearEstimate"] = energyData.monthlyConsumption * 12.0;

  String message;
  size_t bytesWritten = serializeJson(doc, message);
  
  if (bytesWritten == 0 || message.length() == 0) {
    Serial.println("[WEB] ERROR: Failed to serialize broadcast data");
    return;
  }
  
  // Broadcast with error handling
  try {
    webSocket.broadcastTXT(message);
  } catch (...) {
    Serial.println("[WEB] ERROR: Exception during WebSocket broadcast");
  }
}

void WebServerManager::broadcastStatus(const String& message) {
  // Check memory before broadcast
  if (ESP.getFreeHeap() < 5000) {
    return;
  }
  
  DynamicJsonDocument doc(256);
  doc["type"] = "status";
  doc["message"] = message;

  String output;
  size_t bytesWritten = serializeJson(doc, output);
  
  if (bytesWritten == 0 || output.length() == 0) {
    return;
  }
  
  try {
    webSocket.broadcastTXT(output);
  } catch (...) {
    Serial.println("[WEB] ERROR: Exception during status broadcast");
  }
}

void WebServerManager::notifyACActivated() {
  // Check memory before broadcast
  if (ESP.getFreeHeap() < 5000) {
    LOG_WARNING("Web server: Low memory, skipping AC activation notification");
    return;
  }
  
  DynamicJsonDocument doc(128);
  doc["type"] = "acActivated";

  String message;
  size_t bytesWritten = serializeJson(doc, message);
  
  if (bytesWritten == 0 || message.length() == 0) {
    Serial.println("[WEB] ERROR: Failed to serialize AC activation notification");
    return;
  }
  
  try {
    webSocket.broadcastTXT(message);
    Serial.println("[WEB] Sent AC activation notification to all WebSocket clients");
  } catch (...) {
    Serial.println("[WEB] ERROR: Exception during AC activation broadcast");
  }
}

String WebServerManager::generateConfigHTML() {
  String html = "<!DOCTYPE html><html><head><title>Configuration</title></head>";
  html += "<body><h1>System Configuration</h1>";
  html += "<p>Configuration interface coming soon...</p>";
  html += "</body></html>";
  return html;
}

String WebServerManager::generateWiFiConfigHTML() {
  String html = "<!DOCTYPE html><html><head><title>WiFi Configuration</title></head>";
  html += "<body><h1>WiFi Configuration</h1>";
  html += "<form method='POST'>";
  html += "SSID: <input type='text' name='ssid'><br>";
  html += "Password: <input type='password' name='password'><br>";
  html += "<input type='submit' value='Save'>";
  html += "</form></body></html>";
  return html;
}