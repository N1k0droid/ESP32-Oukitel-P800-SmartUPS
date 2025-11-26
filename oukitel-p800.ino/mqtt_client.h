/*
 * MQTT Client Manager - Handles MQTT communication for Home Assistant integration
 */


#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H


#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>


class MQTTClientManager {
private:
  WiFiClient wifiClient;
  PubSubClient mqttClient;
  
  MQTTConfig config;
  bool initialized;
  bool connected;
  unsigned long lastReconnectAttempt;
  unsigned long lastHeartbeat;
  
  // Topic structure
  String baseTopic;
  String stateTopic;
  String commandTopic;
  String availabilityTopic;
  
  // Home Assistant discovery
  void publishDiscoveryConfig();
  void publishSensorDiscovery(const String& name, const String& deviceClass, const String& unit, const String& stateTopic);
  void publishBinaryDiscovery(const String& name, const String& deviceClass, const String& stateTopic);
  void publishButtonDiscovery(const String& name, const String& commandTopic);
  void publishSwitchDiscovery(const String& name, const String& commandTopic, const String& stateTopic);
  
  // Message handlers
  void messageCallback(char* topic, byte* payload, unsigned int length);
  static void messageCallbackStatic(char* topic, byte* payload, unsigned int length);
  
  // Connection management
  bool reconnect();
  void loadConfig();
  void saveConfig();


public:
  MQTTClientManager();
  bool begin();
  void loop();
  bool isConnected();
  
  // Configuration
  bool setConfig(const MQTTConfig& newConfig);
  MQTTConfig getConfig();
  
  // Publishing methods
  void publishData(const SensorData& sensorData, const EnergyData& energyData);
  void publishStatus(const String& status);
  void publishAvailability(bool online);
  
  // ===================================================================
  // MAC ADDRESS SYNCHRONIZATION (NEW! - FIX #3)
  // ===================================================================
  void updateClientIdWithMAC();  // Ricalibbra clientId con vero MAC address
  
  // Static instance for callback
  static MQTTClientManager* instance;
};


#endif // MQTT_CLIENT_H
