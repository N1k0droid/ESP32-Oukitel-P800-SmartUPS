/*
 * HTTP Client Manager - Handles HTTP communication with Home Assistant
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class HTTPClientManager {
private:
  HTTPConfig config;
  bool initialized;
  unsigned long lastPublish;
  
  // Connection management
  void loadConfig();
  void saveConfig();
  bool sendRequest(const String& endpoint, const String& payload);
  
  // Shutdown notification tracking
  bool shutdownNotificationSent;
  void checkBatteryShutdownThreshold(const SensorData& sensorData);
  bool sendShutdownNotification();

public:
  HTTPClientManager();
  bool begin();
  void loop();
  bool isEnabled();
  
  // Configuration
  bool setConfig(const HTTPConfig& newConfig);
  HTTPConfig getConfig();
  
  // Publishing methods
  void publishData(const SensorData& sensorData, const EnergyData& energyData);
  bool executeCommand(const String& command, const String& value);
};

#endif // HTTP_CLIENT_H