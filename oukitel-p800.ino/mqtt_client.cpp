/*
 * MQTT Client Manager Implementation
 */


#include "mqtt_client.h"
#include "hardware_manager.h"
#include "logger.h"
#include <SPIFFS.h>


// Static instance pointer
MQTTClientManager* MQTTClientManager::instance = nullptr;


// External references
extern HardwareManager hardware;


MQTTClientManager::MQTTClientManager() : mqttClient(wifiClient) {
  initialized = false;
  connected = false;
  lastReconnectAttempt = 0;
  lastHeartbeat = 0;
  instance = this;
  
  // Initialize config with defaults
  config.server = "";
  config.port = MQTT_PORT;
  config.username = "";
  config.password = "";
  
  // Get MAC address - use placeholder if WiFi not initialized yet
  String macAddr = WiFi.macAddress();
  if (macAddr == "00:00:00:00:00:00" || macAddr.length() == 0) {
    // WiFi not initialized, use placeholder that will be updated later
    config.clientId = "oukitel_p800e_placeholder";
    baseTopic = String(MQTT_TOPIC_PREFIX) + "/placeholder";
  } else {
    config.clientId = "oukitel_p800e_" + macAddr;
    baseTopic = String(MQTT_TOPIC_PREFIX) + "/" + macAddr;
    baseTopic.replace(":", "");
    baseTopic.toLowerCase();
  }
  
  config.enabled = false;
  
  // Setup topics
  stateTopic = baseTopic + "/state";
  commandTopic = baseTopic + "/command";
  availabilityTopic = baseTopic + "/availability";
}


bool MQTTClientManager::begin() {
  Serial.println("[MQTT] Initializing MQTT client...");
  
  loadConfig();
  
  if (!config.enabled || config.server.length() == 0) {
    Serial.println("[MQTT] MQTT disabled or not configured");
    return true;
  }
  
  mqttClient.setServer(config.server.c_str(), config.port);
  mqttClient.setCallback(messageCallbackStatic);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE);
  mqttClient.setBufferSize(1024);
  
  Serial.println("[MQTT] MQTT client configured");
  Serial.println("  Server: " + config.server + ":" + String(config.port));
  Serial.println("  Client ID: " + config.clientId);
  Serial.println("  Base Topic: " + baseTopic);
  
  initialized = true;
  return true;
}


void MQTTClientManager::loop() {
  if (!initialized || !config.enabled) return;
  
  if (!mqttClient.connected()) {
    connected = false;
    
    // Try to reconnect every 5 seconds
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
    
    if (!connected) {
      connected = true;
      Serial.println("[MQTT] Connected to MQTT broker");
      publishAvailability(true);
      publishDiscoveryConfig();
    }
    
    // Send heartbeat every 30 seconds
    if (millis() - lastHeartbeat > 30000) {
      publishAvailability(true);
      lastHeartbeat = millis();
    }
  }
}


bool MQTTClientManager::reconnect() {
  Serial.println("[MQTT] Attempting MQTT connection...");
  
  // Create will message for availability
  String willTopic = availabilityTopic;
  String willMessage = "offline";
  
  bool connected = false;
  if (config.username.length() > 0) {
    connected = mqttClient.connect(
      config.clientId.c_str(),
      config.username.c_str(),
      config.password.c_str(),
      willTopic.c_str(),
      MQTT_QOS,
      MQTT_RETAIN,
      willMessage.c_str()
    );
  } else {
    connected = mqttClient.connect(
      config.clientId.c_str(),
      willTopic.c_str(),
      MQTT_QOS,
      MQTT_RETAIN,
      willMessage.c_str()
    );
  }
  
  if (connected) {
    Serial.println("[MQTT] MQTT connected");
    
    // Subscribe to command topics for output control
    String usbTopic = commandTopic + "/usb";
    String dcTopic = commandTopic + "/dc";
    String acTopic = commandTopic + "/ac";
    String flashTopic = commandTopic + "/flashlight";
    String powerTopic = commandTopic + "/power";
    
    mqttClient.subscribe(usbTopic.c_str(), MQTT_QOS);
    mqttClient.subscribe(dcTopic.c_str(), MQTT_QOS);
    mqttClient.subscribe(acTopic.c_str(), MQTT_QOS);
    mqttClient.subscribe(flashTopic.c_str(), MQTT_QOS);
    mqttClient.subscribe(powerTopic.c_str(), MQTT_QOS);
    
    LOG_DEBUG("MQTT: Subscribed to command topics");
    
    return true;
  } else {
    LOG_ERROR("MQTT connection failed, rc=" + String(mqttClient.state()));
    return false;
  }
}


void MQTTClientManager::publishDiscoveryConfig() {
  Serial.println("[MQTT] Publishing Home Assistant discovery configuration...");
  
  String deviceId = WiFi.macAddress();
  deviceId.replace(":", "");
  deviceId.toLowerCase();
  
  // Publish sensor discoveries - ALL METRICS
  publishSensorDiscovery("voltage", "voltage", "V", stateTopic + "/voltage");
  publishSensorDiscovery("soc", "battery", "%", stateTopic + "/soc");
  publishSensorDiscovery("main_current", "current", "A", stateTopic + "/main_current");
  publishSensorDiscovery("output_current", "current", "A", stateTopic + "/output_current");
  publishSensorDiscovery("main_power", "power", "W", stateTopic + "/main_power");
  publishSensorDiscovery("output_power", "power", "W", stateTopic + "/output_power");
  publishSensorDiscovery("instant_power", "power", "W", stateTopic + "/instant_power");
  publishSensorDiscovery("daily_consumption", "energy", "kWh", stateTopic + "/daily_consumption");
  publishSensorDiscovery("monthly_consumption", "energy", "kWh", stateTopic + "/monthly_consumption");
  publishSensorDiscovery("peak_power", "power", "W", stateTopic + "/peak_power");
  publishSensorDiscovery("operating_time", "duration", "s", stateTopic + "/operating_time");
  
  // Publish binary sensor discoveries
  publishBinaryDiscovery("on_battery", "power", stateTopic + "/on_battery");
  
  // Publish switch discoveries for output control
  publishSwitchDiscovery("usb_output", commandTopic + "/usb", stateTopic + "/usb");
  publishSwitchDiscovery("dc_output", commandTopic + "/dc", stateTopic + "/dc");
  publishSwitchDiscovery("ac_output", commandTopic + "/ac", stateTopic + "/ac");
  publishSwitchDiscovery("flashlight", commandTopic + "/flashlight", stateTopic + "/flashlight");
  
  Serial.println("[MQTT] Discovery configuration published");
}


void MQTTClientManager::publishSensorDiscovery(const String& name, const String& deviceClass, const String& unit, const String& stateTopic) {
  String topic = "homeassistant/sensor/" + baseTopic + "_" + name + "/config";
  
  // Increased size to prevent overflow (was 1024)
  DynamicJsonDocument doc(1536);
  doc["name"] = "Oukitel P800E " + name;
  doc["unique_id"] = baseTopic + "_" + name;
  doc["state_topic"] = stateTopic;
  doc["device_class"] = deviceClass;
  doc["unit_of_measurement"] = unit;
  doc["availability_topic"] = availabilityTopic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  
  // Device info
  doc["device"]["identifiers"][0] = baseTopic;
  doc["device"]["name"] = "Oukitel P800E";
  doc["device"]["model"] = "P800E";
  doc["device"]["manufacturer"] = "Oukitel";
  doc["device"]["sw_version"] = FIRMWARE_VERSION;
  
  String payload;
  size_t bytesWritten = serializeJson(doc, payload);
  
  // Check for overflow
  if (bytesWritten == 0 || payload.length() == 0) {
    LOG_ERROR("MQTT: Failed to serialize sensor discovery JSON for " + name);
    return;
  }
  
  if (payload.length() > 1400) {
    LOG_WARNING("MQTT: Sensor discovery payload large (" + String(payload.length()) + " bytes) for " + name);
  }
  
  mqttClient.publish(topic.c_str(), payload.c_str(), MQTT_RETAIN);
}


void MQTTClientManager::publishBinaryDiscovery(const String& name, const String& deviceClass, const String& stateTopic) {
  String topic = "homeassistant/binary_sensor/" + baseTopic + "_" + name + "/config";
  
  // Increased size to prevent overflow (was 1024)
  DynamicJsonDocument doc(1536);
  doc["name"] = "Oukitel P800E " + name;
  doc["unique_id"] = baseTopic + "_" + name;
  doc["state_topic"] = stateTopic;
  doc["device_class"] = deviceClass;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["availability_topic"] = availabilityTopic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  
  // Device info
  doc["device"]["identifiers"][0] = baseTopic;
  doc["device"]["name"] = "Oukitel P800E";
  doc["device"]["model"] = "P800E";
  doc["device"]["manufacturer"] = "Oukitel";
  doc["device"]["sw_version"] = FIRMWARE_VERSION;
  
  String payload;
  size_t bytesWritten = serializeJson(doc, payload);
  
  if (bytesWritten == 0 || payload.length() == 0) {
    LOG_ERROR("MQTT: Failed to serialize binary discovery JSON for " + name);
    return;
  }
  
  mqttClient.publish(topic.c_str(), payload.c_str(), MQTT_RETAIN);
}


void MQTTClientManager::publishSwitchDiscovery(const String& name, const String& commandTopic, const String& stateTopic) {
  String topic = "homeassistant/switch/" + baseTopic + "_" + name + "/config";
  
  // Increased size to prevent overflow (was 1024)
  DynamicJsonDocument doc(1536);
  doc["name"] = "Oukitel P800E " + name;
  doc["unique_id"] = baseTopic + "_" + name;
  doc["command_topic"] = commandTopic;
  doc["state_topic"] = stateTopic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["availability_topic"] = availabilityTopic;
  doc["payload_available"] = "online";
  doc["payload_not_available"] = "offline";
  
  // Device info
  doc["device"]["identifiers"][0] = baseTopic;
  doc["device"]["name"] = "Oukitel P800E";
  doc["device"]["model"] = "P800E";
  doc["device"]["manufacturer"] = "Oukitel";
  doc["device"]["sw_version"] = FIRMWARE_VERSION;
  
  String payload;
  size_t bytesWritten = serializeJson(doc, payload);
  
  if (bytesWritten == 0 || payload.length() == 0) {
    LOG_ERROR("MQTT: Failed to serialize switch discovery JSON for " + name);
    return;
  }
  
  mqttClient.publish(topic.c_str(), payload.c_str(), MQTT_RETAIN);
}


void MQTTClientManager::messageCallbackStatic(char* topic, byte* payload, unsigned int length) {
  if (instance) {
    instance->messageCallback(topic, payload, length);
  }
}


void MQTTClientManager::messageCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  String payloadStr = "";
  
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  
  Serial.println("[MQTT] Message received: " + topicStr + " = " + payloadStr);
  
  // Handle output control commands
  if (topicStr == commandTopic + "/usb") {
    if (payloadStr == "ON" || payloadStr == "1") {
      hardware.pressButton(BTN_USB, BUTTON_STANDARD_DURATION);
      Serial.println("[MQTT] USB output toggled via MQTT");
    }
  } else if (topicStr == commandTopic + "/dc") {
    if (payloadStr == "ON" || payloadStr == "1") {
      hardware.pressButton(BTN_DC, BUTTON_STANDARD_DURATION);
      Serial.println("[MQTT] DC output toggled via MQTT");
    }
  } else if (topicStr == commandTopic + "/ac") {
    if (payloadStr == "ON" || payloadStr == "1") {
      hardware.pressButton(BTN_AC, BUTTON_STANDARD_DURATION);
      Serial.println("[MQTT] AC output toggled via MQTT");
    }
  } else if (topicStr == commandTopic + "/flashlight") {
    if (payloadStr == "ON" || payloadStr == "1") {
      hardware.pressButton(BTN_FLASHLIGHT, BUTTON_STANDARD_DURATION);
      Serial.println("[MQTT] Flashlight toggled via MQTT");
    }
  } else if (topicStr == commandTopic + "/power") {
    if (payloadStr == "ON" || payloadStr == "1") {
      hardware.pressButton(BTN_POWER, BUTTON_POWER_DURATION);
      Serial.println("[MQTT] Power button pressed via MQTT");
    }
  }
}


void MQTTClientManager::publishData(const SensorData& sensorData, const EnergyData& energyData) {
  if (!connected) return;
  
  // Publish ALL individual sensor values
  mqttClient.publish((stateTopic + "/voltage").c_str(), String(sensorData.batteryVoltage, 2).c_str());
  mqttClient.publish((stateTopic + "/soc").c_str(), String(sensorData.batteryPercentage, 1).c_str());
  mqttClient.publish((stateTopic + "/main_current").c_str(), String(sensorData.mainCurrent, 2).c_str());
  mqttClient.publish((stateTopic + "/output_current").c_str(), String(sensorData.outputCurrent, 2).c_str());
  mqttClient.publish((stateTopic + "/main_power").c_str(), String(sensorData.mainPower, 0).c_str());
  mqttClient.publish((stateTopic + "/output_power").c_str(), String(sensorData.outputPower, 0).c_str());
  mqttClient.publish((stateTopic + "/on_battery").c_str(), sensorData.onBattery ? "ON" : "OFF");
  
  // Publish ALL energy data
  mqttClient.publish((stateTopic + "/instant_power").c_str(), String(energyData.instantPower, 0).c_str());
  mqttClient.publish((stateTopic + "/daily_consumption").c_str(), String(energyData.dailyConsumption, 3).c_str());
  mqttClient.publish((stateTopic + "/monthly_consumption").c_str(), String(energyData.monthlyConsumption, 3).c_str());
  mqttClient.publish((stateTopic + "/peak_power").c_str(), String(energyData.peakPower, 0).c_str());
  mqttClient.publish((stateTopic + "/operating_time").c_str(), String(energyData.operatingTime).c_str());
  
  // Publish combined JSON payload - increased size for safety
  DynamicJsonDocument doc(1536);
  doc["voltage"] = sensorData.batteryVoltage;
  doc["soc"] = sensorData.batteryPercentage;
  doc["main_current"] = sensorData.mainCurrent;
  doc["output_current"] = sensorData.outputCurrent;
  doc["main_power"] = sensorData.mainPower;
  doc["output_power"] = sensorData.outputPower;
  doc["on_battery"] = sensorData.onBattery;
  doc["battery_state"] = sensorData.batteryState;
  doc["instant_power"] = energyData.instantPower;
  doc["daily_consumption"] = energyData.dailyConsumption;
  doc["monthly_consumption"] = energyData.monthlyConsumption;
  doc["peak_power"] = energyData.peakPower;
  doc["operating_time"] = energyData.operatingTime;
  doc["timestamp"] = sensorData.timestamp;
  
  String payload;
  size_t bytesWritten = serializeJson(doc, payload);
  
  if (bytesWritten == 0 || payload.length() == 0) {
    LOG_ERROR("MQTT: Failed to serialize data JSON");
    return;
  }
  
  mqttClient.publish(stateTopic.c_str(), payload.c_str());
}


void MQTTClientManager::publishStatus(const String& status) {
  if (!connected) return;
  
  mqttClient.publish((baseTopic + "/status").c_str(), status.c_str());
}


void MQTTClientManager::publishAvailability(bool online) {
  String payload = online ? "online" : "offline";
  mqttClient.publish(availabilityTopic.c_str(), payload.c_str(), MQTT_RETAIN);
}


bool MQTTClientManager::isConnected() {
  return connected && mqttClient.connected();
}


bool MQTTClientManager::setConfig(const MQTTConfig& newConfig) {
  config = newConfig;
  saveConfig();
  
  if (initialized) {
    // Disconnect and reconnect with new config
    mqttClient.disconnect();
    connected = false;
    
    if (config.enabled && config.server.length() > 0) {
      mqttClient.setServer(config.server.c_str(), config.port);
    }
  }
  
  return true;
}


MQTTConfig MQTTClientManager::getConfig() {
  return config;
}


// ===================================================================
// MAC Address Synchronization
// ===================================================================
void MQTTClientManager::updateClientIdWithMAC() {
  String realMAC = WiFi.macAddress();
  
  // Verifica se il MAC è valido (non tutti zeri e non vuoto)
  if (realMAC == "00:00:00:00:00:00" || realMAC.length() == 0) {
    Serial.println("[MQTT] ⚠️ updateClientIdWithMAC() called but MAC is still invalid (" + realMAC + ") - skipping");
    return;
  }
  
  // Se il clientId contiene "placeholder" o è diverso dal MAC reale, aggiornalo
  String newClientId = "oukitel_p800e_" + realMAC;
  bool needsUpdate = false;
  
  if (config.clientId != newClientId) {
    // Check if current clientId contains placeholder or old MAC
    if (config.clientId.indexOf("placeholder") >= 0 || 
        config.clientId != newClientId) {
      needsUpdate = true;
    }
  }
  
  if (needsUpdate) {
    Serial.println("[MQTT] ✓ MAC Address updated: " + config.clientId + " → " + newClientId);
    config.clientId = newClientId;
    saveConfig();
    
    // Aggiorna anche il baseTopic
    String newBaseTopic = String(MQTT_TOPIC_PREFIX) + "/" + realMAC;
    newBaseTopic.replace(":", "");
    newBaseTopic.toLowerCase();
    
    if (baseTopic != newBaseTopic) {
      Serial.println("[MQTT] ✓ Base Topic updated: " + baseTopic + " → " + newBaseTopic);
      baseTopic = newBaseTopic;
      stateTopic = baseTopic + "/state";
      commandTopic = baseTopic + "/command";
      availabilityTopic = baseTopic + "/availability";
    }
    
    // Se MQTT è connesso, disconnetti e riconnetti con nuovo clientId
    if (mqttClient.connected()) {
      Serial.println("[MQTT] Disconnecting to reconnect with new clientId...");
      mqttClient.disconnect();
      connected = false;
      lastReconnectAttempt = 0;  // Force immediate reconnect attempt
    }
  } else {
    Serial.println("[MQTT] updateClientIdWithMAC() called - already synchronized with real MAC");
  }
}


void MQTTClientManager::loadConfig() {
  if (!SPIFFS.exists("/mqtt_config.json")) {
    Serial.println("[MQTT] No saved MQTT configuration found");
    return;
  }
  
  File file = SPIFFS.open("/mqtt_config.json", "r");
  if (!file) {
    LOG_ERROR("MQTT: Failed to open config file");
    return;
  }
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    LOG_ERROR("MQTT: Failed to parse config file");
    return;
  }
  
  config.server = doc["server"].as<String>();
  config.port = doc["port"] | MQTT_PORT;
  config.username = doc["username"].as<String>();
  config.password = doc["password"].as<String>();
  config.clientId = doc["clientId"].as<String>();
  config.enabled = doc["enabled"] | false;
  
  if (config.clientId.length() == 0) {
    config.clientId = "oukitel_p800e_" + WiFi.macAddress();
  }
  
  Serial.println("[MQTT] Loaded MQTT configuration");
}


void MQTTClientManager::saveConfig() {
  DynamicJsonDocument doc(1024);
  doc["server"] = config.server;
  doc["port"] = config.port;
  doc["username"] = config.username;
  doc["password"] = config.password;
  doc["clientId"] = config.clientId;
  doc["enabled"] = config.enabled;
  
  File file = SPIFFS.open("/mqtt_config.json", "w");
  if (!file) {
    LOG_ERROR("MQTT: Failed to save configuration");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("[MQTT] MQTT configuration saved");
}
