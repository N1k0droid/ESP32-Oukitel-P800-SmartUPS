/*
 * Web Server Manager - Handles HTTP and WebSocket communication
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declarations
class HardwareManager;

class WebServerManager {
private:
  WebServer server;
  WebSocketsServer webSocket;
  bool initialized;
  unsigned long lastDataBroadcast;
  
  static WebServerManager* instance;
  static HardwareManager* hwManager;
  
  void handleRoot();
  void handleAPI();
  void handleAPICommand();
  void handleConfig();
  void handleWiFiConfig();
  void handleButtonPress();
  void handleNotFound();
  void sendCORS();
  bool validateAPIPassword(const String& password);
  
  static void webSocketEventStatic(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
  void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
  
  String generateHTML();
  String generateConfigHTML();
  String generateWiFiConfigHTML();
  
public:
  WebServerManager();
  bool begin();
  void handleClient();
  void broadcastData(const SensorData& sensorData, const EnergyData& energyData);
  void broadcastStatus(const String& message);
  void setHardwareManager(HardwareManager* hw);
  void notifyACActivated();
};

#endif // WEB_SERVER_H