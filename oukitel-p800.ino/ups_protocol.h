/*
 * UPS Protocol Implementation - Compatible with Proxmox and NUT
 */

#ifndef UPS_PROTOCOL_H
#define UPS_PROTOCOL_H

#include "config.h"
#include <WiFi.h>

// UPSConfig is defined in config.h

class UPSProtocol {
private:
  WiFiServer* server;
  uint16_t currentPort;  // Track current server port to compare with config
  WiFiClient clients[UPS_MAX_CLIENTS];
  unsigned long clientLastActivity[UPS_MAX_CLIENTS];  // Track last activity for timeout
  
  UPSConfig config;
  SystemStatus currentStatus;
  SensorData lastSensorData;
  bool shutdownRequested;
  unsigned long shutdownTime;
  
  // UPS variables for NUT protocol
  String upsName;
  String upsDescription;
  String manufacturer;
  String model;
  String serial;
  
  void handleClient(WiFiClient& client);
  void sendResponse(WiFiClient& client, const String& response);
  String processCommand(const String& command);
  
  // NUT protocol commands
  String handleListUPS();
  String handleListVar(const String& upsName);
  String handleGetVar(const String& upsName, const String& varName);
  String handleInstCmd(const String& upsName, const String& command);
  String handleListCmd(const String& upsName);
  
  // Status determination
  SystemStatus determineStatus(const SensorData& data);
  String getStatusString();
  
  // Configuration
  void loadConfig();
  void saveConfig();
  
public:
  UPSProtocol();
  ~UPSProtocol();
  bool begin();
  void handleClients();
  void updateStatus(const SensorData& sensorData);
  
  // Status methods
  SystemStatus getStatus();
  bool isShutdownRequested();
  void requestShutdown(int delaySeconds = 0);
  void cancelShutdown();
  
  // Configuration
  void setUPSInfo(const String& name, const String& desc, const String& mfr, const String& mdl);
  bool setConfig(const UPSConfig& newConfig);
  UPSConfig getConfig();
  bool isEnabled();
};

#endif // UPS_PROTOCOL_H