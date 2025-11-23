/*
 * UPS Protocol Implementation
 */

#include "ups_protocol.h"
#include "logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

UPSProtocol::UPSProtocol() : server(nullptr), currentPort(0) {
  currentStatus = STATUS_INITIALIZING;
  shutdownRequested = false;
  shutdownTime = 0;
  
  upsName = "oukitel_p800e";
  upsDescription = "Oukitel P800E Power Station";
  manufacturer = "Oukitel";
  model = "P800E";
  serial = WiFi.macAddress();
  
  // Default config
  config.enabled = true;
  config.port = UPS_PORT;
  config.shutdownThreshold = UPS_SHUTDOWN_THRESHOLD;
  
  // Initialize client activity tracking
  for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
    clientLastActivity[i] = 0;
  }
}

UPSProtocol::~UPSProtocol() {
  if (server) {
    delete server;
  }
}

bool UPSProtocol::begin() {
  loadConfig();
  
  if (!config.enabled) {
    Serial.println("[UPS] UPS protocol disabled");
    return true;
  }
  
  Serial.println("[UPS] Starting UPS protocol server on port " + String(config.port));
  Serial.println("[UPS] UPS Name: " + upsName);
  Serial.println("[UPS] UPS Description: " + upsDescription);
  
  // Check memory before allocating server
  if (ESP.getFreeHeap() < 10000) {
    Serial.println("[UPS] ERROR: Insufficient memory to start server");
    return false;
  }
  
  // Allocate server with error handling
  WiFiServer* newServer = new WiFiServer(config.port);
  if (!newServer) {
    Serial.println("[UPS] ERROR: Failed to allocate WiFiServer");
    return false;
  }
  
  // Try to start server
  newServer->begin();
  newServer->setNoDelay(true);
  
  // If we had an old server, delete it now (after new one is working)
  if (server) {
    delete server;
  }
  server = newServer;
  currentPort = config.port;  // Save current port
  
  // Initialize client array - ensure all are properly reset
  for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
    if (clients[i]) {
      clients[i].stop();
    }
    clients[i] = WiFiClient();
  }
  
  currentStatus = STATUS_NORMAL;
  Serial.println("[UPS] UPS protocol server started");
  return true;
}

void UPSProtocol::handleClients() {
  if (!config.enabled || !server) return;
  
  // Check for new clients
  WiFiClient newClient = server->available();
  if (newClient) {
    // Find empty slot
    for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
      if (!clients[i] || !clients[i].connected()) {
        clients[i] = newClient;
        clientLastActivity[i] = millis();  // Initialize activity time
        Serial.println("[UPS] New client connected: " + newClient.remoteIP().toString());
        sendResponse(clients[i], "Network UPS Tools upsd 2.7.4 - http://www.networkupstools.org/");
        break;
      }
    }
  }
  
  // Handle existing clients with timeout check
  unsigned long currentTime = millis();
  for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      // Check for timeout (30 seconds of inactivity)
      if (clientLastActivity[i] == 0) {
        clientLastActivity[i] = currentTime;
      }
      
      if (clients[i].available()) {
        clientLastActivity[i] = currentTime;  // Reset timeout on activity
        String command = clients[i].readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
          Serial.println("[UPS] Command from client " + String(i) + ": " + command);
          String response = processCommand(command);
          sendResponse(clients[i], response);
        }
      } else if (currentTime - clientLastActivity[i] > 30000) {
        // Client timeout - disconnect (handles millis() overflow)
        if ((currentTime - clientLastActivity[i]) < 0x7FFFFFFF) {  // Check for overflow
          Serial.println("[UPS] Client " + String(i) + " timeout, disconnecting");
          clients[i].stop();
          clientLastActivity[i] = 0;
        }
      }
    } else if (clients[i]) {
      // Client disconnected
      Serial.println("[UPS] Client " + String(i) + " disconnected");
      clients[i].stop();
      clientLastActivity[i] = 0;
    }
  }
  
  // Handle shutdown request
  if (shutdownRequested && shutdownTime > 0 && millis() >= shutdownTime) {
    Serial.println("[UPS] Executing shutdown command");
    // Broadcast shutdown message to all clients
    for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
      if (clients[i] && clients[i].connected()) {
        sendResponse(clients[i], "NOTIFY " + upsName + " SHUTDOWN");
      }
    }
    shutdownRequested = false;
    shutdownTime = 0;
  }
}

void UPSProtocol::updateStatus(const SensorData& sensorData) {
  if (!config.enabled) return;
  
  lastSensorData = sensorData;
  SystemStatus newStatus = determineStatus(sensorData);
  
  if (newStatus != currentStatus) {
    Serial.println("[UPS] Status changed: " + String(currentStatus) + " -> " + String(newStatus));
    currentStatus = newStatus;
    
    // Notify clients of status change
    String notification = "NOTIFY " + upsName + " " + getStatusString();
    for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
      if (clients[i] && clients[i].connected()) {
        sendResponse(clients[i], notification);
      }
    }
    
    // Auto-shutdown on critical battery
    if (currentStatus == STATUS_CRITICAL_BATTERY && !shutdownRequested) {
      Serial.println("[UPS] Critical battery level, requesting shutdown");
      requestShutdown(30); // 30 second delay
    }
  }
}

SystemStatus UPSProtocol::determineStatus(const SensorData& data) {
  if (data.batteryPercentage <= config.shutdownThreshold) {
    return STATUS_CRITICAL_BATTERY;
  } else if (data.batteryPercentage <= 70 && data.onBattery) {
    return STATUS_LOW_BATTERY;
  } else if (data.onBattery) {
    return STATUS_ON_BATTERY;
  } else {
    return STATUS_NORMAL;
  }
}

String UPSProtocol::getStatusString() {
  switch (currentStatus) {
    case STATUS_NORMAL:
      return "OL"; // On Line
    case STATUS_ON_BATTERY:
      return "OB"; // On Battery
    case STATUS_LOW_BATTERY:
      return "OB LB"; // On Battery, Low Battery
    case STATUS_CRITICAL_BATTERY:
      return "OB LB RB"; // On Battery, Low Battery, Replace Battery
    case STATUS_ERROR:
      return "ALARM";
    default:
      return "OL";
  }
}

void UPSProtocol::sendResponse(WiFiClient& client, const String& response) {
  if (client && client.connected()) {
    client.println(response);
    client.flush();
  }
}

String UPSProtocol::processCommand(const String& command) {
  String cmd = command;
  cmd.toUpperCase();
  
  if (cmd.startsWith("LIST UPS")) {
    return handleListUPS();
  } else if (cmd.startsWith("LIST VAR")) {
    String upsName = command.substring(9);
    upsName.trim();
    return handleListVar(upsName);
  } else if (cmd.startsWith("GET VAR")) {
    int firstSpace = command.indexOf(' ', 8);
    if (firstSpace > 0) {
      String upsName = command.substring(8, firstSpace);
      String varName = command.substring(firstSpace + 1);
      upsName.trim();
      varName.trim();
      return handleGetVar(upsName, varName);
    }
  } else if (cmd.startsWith("INST CMD")) {
    int firstSpace = command.indexOf(' ', 9);
    if (firstSpace > 0) {
      String upsName = command.substring(9, firstSpace);
      String cmdName = command.substring(firstSpace + 1);
      upsName.trim();
      cmdName.trim();
      return handleInstCmd(upsName, cmdName);
    }
  } else if (cmd.startsWith("LIST CMD")) {
    String upsName = command.substring(9);
    upsName.trim();
    return handleListCmd(upsName);
  } else if (cmd == "VER") {
    return "Network UPS Tools upsd 2.7.4";
  } else if (cmd == "HELP") {
    return "Commands: LIST UPS, LIST VAR <ups>, GET VAR <ups> <var>, INST CMD <ups> <cmd>, LIST CMD <ups>, VER, HELP";
  }
  
  return "ERR UNKNOWN-COMMAND";
}

String UPSProtocol::handleListUPS() {
  return "UPS " + upsName + " \"" + upsDescription + "\"";
}

String UPSProtocol::handleListVar(const String& upsName) {
  if (upsName != this->upsName) {
    return "ERR UNKNOWN-UPS";
  }
  
  String response = "BEGIN LIST VAR " + upsName + "\n";
  response += "VAR " + upsName + " ups.status \"" + getStatusString() + "\"\n";
  response += "VAR " + upsName + " battery.charge \"" + String(lastSensorData.batteryPercentage, 0) + "\"\n";
  response += "VAR " + upsName + " battery.voltage \"" + String(lastSensorData.batteryVoltage, 1) + "\"\n";
  response += "VAR " + upsName + " input.current \"" + String(lastSensorData.mainCurrent, 2) + "\"\n";
  response += "VAR " + upsName + " output.current \"" + String(lastSensorData.outputCurrent, 2) + "\"\n";
  response += "VAR " + upsName + " ups.load \"" + String((lastSensorData.outputPower / 2400.0) * 100, 1) + "\"\n";
  response += "VAR " + upsName + " ups.power \"" + String(lastSensorData.outputPower, 0) + "\"\n";
  response += "VAR " + upsName + " ups.mfr \"" + manufacturer + "\"\n";
  response += "VAR " + upsName + " ups.model \"" + model + "\"\n";
  response += "VAR " + upsName + " ups.serial \"" + serial + "\"\n";
  response += "VAR " + upsName + " device.type \"ups\"\n";
  response += "END LIST VAR " + upsName;
  
  return response;
}

String UPSProtocol::handleGetVar(const String& upsName, const String& varName) {
  if (upsName != this->upsName) {
    return "ERR UNKNOWN-UPS";
  }
  
  if (varName == "ups.status") {
    return "VAR " + upsName + " " + varName + " \"" + getStatusString() + "\"";
  } else if (varName == "battery.charge") {
    return "VAR " + upsName + " " + varName + " \"" + String(lastSensorData.batteryPercentage, 0) + "\"";
  } else if (varName == "battery.voltage") {
    return "VAR " + upsName + " " + varName + " \"" + String(lastSensorData.batteryVoltage, 1) + "\"";
  } else if (varName == "input.current") {
    return "VAR " + upsName + " " + varName + " \"" + String(lastSensorData.mainCurrent, 2) + "\"";
  } else if (varName == "output.current") {
    return "VAR " + upsName + " " + varName + " \"" + String(lastSensorData.outputCurrent, 2) + "\"";
  } else if (varName == "ups.load") {
    return "VAR " + upsName + " " + varName + " \"" + String((lastSensorData.outputPower / 2400.0) * 100, 1) + "\"";
  } else if (varName == "ups.power") {
    return "VAR " + upsName + " " + varName + " \"" + String(lastSensorData.outputPower, 0) + "\"";
  } else if (varName == "ups.mfr") {
    return "VAR " + upsName + " " + varName + " \"" + manufacturer + "\"";
  } else if (varName == "ups.model") {
    return "VAR " + upsName + " " + varName + " \"" + model + "\"";
  } else if (varName == "ups.serial") {
    return "VAR " + upsName + " " + varName + " \"" + serial + "\"";
  } else if (varName == "device.type") {
    return "VAR " + upsName + " " + varName + " \"ups\"";
  }
  
  return "ERR VAR-NOT-SUPPORTED";
}

String UPSProtocol::handleInstCmd(const String& upsName, const String& command) {
  if (upsName != this->upsName) {
    return "ERR UNKNOWN-UPS";
  }
  
  if (command == "shutdown.return") {
    requestShutdown(10);
    return "OK";
  } else if (command == "shutdown.stop") {
    cancelShutdown();
    return "OK";
  }
  
  return "ERR CMD-NOT-SUPPORTED";
}

String UPSProtocol::handleListCmd(const String& upsName) {
  if (upsName != this->upsName) {
    return "ERR UNKNOWN-UPS";
  }
  
  String response = "BEGIN LIST CMD " + upsName + "\n";
  response += "CMD " + upsName + " shutdown.return\n";
  response += "CMD " + upsName + " shutdown.stop\n";
  response += "END LIST CMD " + upsName;
  
  return response;
}

SystemStatus UPSProtocol::getStatus() {
  return currentStatus;
}

bool UPSProtocol::isShutdownRequested() {
  return shutdownRequested;
}

void UPSProtocol::requestShutdown(int delaySeconds) {
  shutdownRequested = true;
  shutdownTime = millis() + (delaySeconds * 1000);
  
  Serial.println("[UPS] Shutdown requested with " + String(delaySeconds) + " second delay");
  
  // Notify all clients
  String notification = "NOTIFY " + upsName + " SHUTDOWN " + String(delaySeconds);
  for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      sendResponse(clients[i], notification);
    }
  }
}

void UPSProtocol::cancelShutdown() {
  shutdownRequested = false;
  shutdownTime = 0;
  
  Serial.println("[UPS] Shutdown cancelled");
  
  // Notify all clients
  String notification = "NOTIFY " + upsName + " SHUTDOWN-CANCELLED";
  for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
    if (clients[i] && clients[i].connected()) {
      sendResponse(clients[i], notification);
    }
  }
}

void UPSProtocol::setUPSInfo(const String& name, const String& desc, const String& mfr, const String& mdl) {
  upsName = name;
  upsDescription = desc;
  manufacturer = mfr;
  model = mdl;
}

bool UPSProtocol::setConfig(const UPSConfig& newConfig) {
  config = newConfig;
  saveConfig();
  
  // Restart server if port changed or enabled state changed
  bool needsRestart = false;
  if (server && (!config.enabled || config.port != currentPort)) {
    needsRestart = true;
  } else if (!server && config.enabled) {
    needsRestart = true;
  }
  
  if (needsRestart) {
    // Clean up old server properly
    if (server) {
      // Disconnect all clients first
      for (int i = 0; i < UPS_MAX_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
          clients[i].stop();
        }
      }
      delete server;
      server = nullptr;
      currentPort = 0;  // Reset port tracking
    }
    
    // Start new server if enabled
    if (config.enabled) {
      return begin();
    }
  }
  
  return true;
}

UPSConfig UPSProtocol::getConfig() {
  return config;
}

bool UPSProtocol::isEnabled() {
  return config.enabled;
}

void UPSProtocol::loadConfig() {
  if (!SPIFFS.exists("/ups_config.json")) {
    Serial.println("[UPS] No saved UPS configuration found, using defaults");
    return;
  }
  
  File file = SPIFFS.open("/ups_config.json", "r");
  if (!file) {
    Serial.println("[UPS] Failed to open UPS config file");
    return;
  }
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("[UPS] Failed to parse UPS config file");
    return;
  }
  
  config.enabled = doc["enabled"] | true;
  config.port = doc["port"] | UPS_PORT;
  config.shutdownThreshold = doc["shutdownThreshold"] | UPS_SHUTDOWN_THRESHOLD;
  
  Serial.println("[UPS] Loaded UPS configuration");
}

void UPSProtocol::saveConfig() {
  DynamicJsonDocument doc(512);
  doc["enabled"] = config.enabled;
  doc["port"] = config.port;
  doc["shutdownThreshold"] = config.shutdownThreshold;
  
  File file = SPIFFS.open("/ups_config.json", "w");
  if (!file) {
    Serial.println("[UPS] Failed to save UPS configuration");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("[UPS] UPS configuration saved");
}