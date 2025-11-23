/*
 * Web Server Manager - API and WebSocket Events
 */

#include "web_server.h"
#include "hardware_manager.h"
#include "wifi_manager.h"
#include "data_logger.h"
#include "energy_monitor.h"
#include "mqtt_client.h"
#include "http_client.h"
#include "ups_protocol.h"
#include "logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// External declarations
extern HardwareManager hardware;
extern WiFiManager wifiMgr;
extern DataLogger dataLogger;
extern PowerStationMonitor energyMonitor;
extern MQTTClientManager mqttClient;
extern HTTPClientManager httpClient;
extern UPSProtocol upsProtocol;

void WebServerManager::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Client " + String(num) + " disconnected");
      break;

    case WStype_CONNECTED:
      {
        Serial.println("[WS] Client " + String(num) + " connected");

        SensorData data = hardware.getSensorData();
        EnergyData energyData = energyMonitor.getStableEnergyData();

        // Increased size to prevent overflow (was 1024)
        DynamicJsonDocument doc(2048);
        doc["type"] = "sensorData";
        doc["timestamp"] = data.timestamp;
        doc["voltage"] = data.batteryVoltage;
        doc["soc"] = data.batteryPercentage;
        doc["powerIn"] = data.mainPower;
        doc["powerOut"] = data.outputPower;
        doc["state"] = hardware.getStateString(data.batteryState);
        doc["autoPowerOn"] = hardware.getAutoPowerOn();
        doc["heap"] = ESP.getFreeHeap();
        doc["uptime"] = millis() / 1000;
        doc["instantPower"] = energyData.instantPower;
        doc["dailyConsumption"] = energyData.dailyConsumption;
        doc["monthCurrent"] = energyData.monthlyConsumption;
        doc["yearEstimate"] = energyData.monthlyConsumption * 12.0;
        doc["ipAddress"] = WiFi.localIP().toString();
        doc["macAddress"] = WiFi.macAddress();
        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();

        String message;
        size_t bytesWritten = serializeJson(doc, message);
        
        if (bytesWritten == 0 || message.length() == 0) {
          LOG_ERROR("WebSocket: Failed to serialize sensor data for client " + String(num));
          break;
        }
        
        // Check memory before sending
        if (ESP.getFreeHeap() < 5000) {
          LOG_WARNING("WebSocket: Low memory, skipping data send to client " + String(num));
          break;
        }
        
        webSocket.sendTXT(num, message);

        // Send calibration data - increased size for safety
        CalibrationData cal = hardware.getCalibrationData();
        DynamicJsonDocument calDoc(1024);
        calDoc["type"] = "calibrationData";
        calDoc["sct013CalIn"] = cal.sct013CalIn;
        calDoc["sct013OffsetIn"] = cal.sct013OffsetIn;
        calDoc["sct013CalOut"] = cal.sct013CalOut;
        calDoc["sct013OffsetOut"] = cal.sct013OffsetOut;
        calDoc["batteryDividerRatio"] = cal.batteryDividerRatio;
        calDoc["batteryAdcCalibration"] = cal.batteryAdcCalibration;
        calDoc["voltageOffsetCharge"] = cal.voltageOffsetCharge;
        calDoc["voltageOffsetDischarge"] = cal.voltageOffsetDischarge;
        calDoc["voltageOffsetRest"] = cal.voltageOffsetRest;
        calDoc["fixedVoltage"] = cal.fixedVoltage;     // NEW
        calDoc["mainsVoltage"] = cal.mainsVoltage;     // NEW
        String calMessage;
        size_t calBytesWritten = serializeJson(calDoc, calMessage);
        
        if (calBytesWritten > 0 && calMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, calMessage);
        }

        // Send advanced settings
        AdvancedSettings adv = hardware.getAdvancedSettings();
        DynamicJsonDocument advDoc(512);
        advDoc["type"] = "advancedSettings";
        advDoc["powerStationOffVoltage"] = adv.powerStationOffVoltage;
        advDoc["powerThreshold"] = adv.powerThreshold;
        advDoc["powerFilterAlpha"] = adv.powerFilterAlpha;
        advDoc["voltageMinSafe"] = adv.voltageMinSafe;
        advDoc["batteryLowWarning"] = adv.batteryLowWarning;
        advDoc["batteryCritical"] = adv.batteryCritical;
        advDoc["autoPowerOnDelay"] = adv.autoPowerOnDelay;
        advDoc["socBufferSize"] = adv.socBufferSize;
        advDoc["socChangeThreshold"] = adv.socChangeThreshold;
        advDoc["warmupDelay"] = adv.warmupDelay;

        String advMessage;
        size_t advBytesWritten = serializeJson(advDoc, advMessage);
        
        if (advBytesWritten > 0 && advMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, advMessage);
        }
        
        // Send MQTT config
        MQTTConfig mqttConfig = mqttClient.getConfig();
        DynamicJsonDocument mqttDoc(512);
        mqttDoc["type"] = "mqttConfig";
        mqttDoc["enabled"] = mqttConfig.enabled;
        mqttDoc["server"] = mqttConfig.server;
        mqttDoc["port"] = mqttConfig.port;
        mqttDoc["username"] = mqttConfig.username;
        mqttDoc["password"] = mqttConfig.password;
        mqttDoc["clientId"] = mqttConfig.clientId;
        String mqttMessage;
        size_t mqttBytes = serializeJson(mqttDoc, mqttMessage);
        if (mqttBytes > 0 && mqttMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, mqttMessage);
        }
        
        // Send HTTP config
        HTTPConfig httpConfig = httpClient.getConfig();
        DynamicJsonDocument httpDoc(512);
        httpDoc["type"] = "httpConfig";
        httpDoc["enabled"] = httpConfig.enabled;
        httpDoc["server"] = httpConfig.server;
        httpDoc["port"] = httpConfig.port;
        httpDoc["endpoint"] = httpConfig.endpoint;
        httpDoc["apiKey"] = httpConfig.apiKey;
        String httpMessage;
        size_t httpBytes = serializeJson(httpDoc, httpMessage);
        if (httpBytes > 0 && httpMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, httpMessage);
        }
        
        // Send UPS config
        UPSConfig upsConfig = upsProtocol.getConfig();
        DynamicJsonDocument upsDoc(512);
        upsDoc["type"] = "upsConfig";
        upsDoc["enabled"] = upsConfig.enabled;
        upsDoc["port"] = upsConfig.port;
        upsDoc["shutdownThreshold"] = upsConfig.shutdownThreshold;
        String upsMessage;
        size_t upsBytes = serializeJson(upsDoc, upsMessage);
        if (upsBytes > 0 && upsMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, upsMessage);
        }
        
        // Send System Settings (NTP + Beeps) - NEW!
        DynamicJsonDocument sysDoc(512);
        sysDoc["type"] = "systemSettings";
        sysDoc["ntpServer"] = g_ntpServer;
        sysDoc["gmtOffset"] = g_gmtOffset;
        sysDoc["daylightOffset"] = g_daylightOffset;
        sysDoc["beepsEnabled"] = g_beepsEnabled;
        sysDoc["logLevel"] = g_logLevel;
        String sysMessage;
        size_t sysBytes = serializeJson(sysDoc, sysMessage);
        if (sysBytes > 0 && sysMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, sysMessage);
        }
        
        // Send HTTP Shutdown Config - NEW!
        DynamicJsonDocument shutdownDoc(512);
        shutdownDoc["type"] = "httpShutdownConfig";
        shutdownDoc["enabled"] = g_httpShutdownEnabled;
        shutdownDoc["batteryThreshold"] = g_httpShutdownThreshold;
        shutdownDoc["server"] = g_httpShutdownServer;
        shutdownDoc["port"] = g_httpShutdownPort;
        shutdownDoc["password"] = g_httpShutdownPassword;
        String shutdownMessage;
        size_t shutdownBytes = serializeJson(shutdownDoc, shutdownMessage);
        if (shutdownBytes > 0 && shutdownMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, shutdownMessage);
        }
        
        // Send monthly history
        std::vector<MonthlyEnergyRecord> history = energyMonitor.getMonthlyHistory();
        DynamicJsonDocument histDoc(2048);
        histDoc["type"] = "monthlyHistory";
        JsonArray histArray = histDoc.createNestedArray("history");
        for (const auto& record : history) {
          JsonObject obj = histArray.createNestedObject();
          obj["year"] = record.year;
          obj["month"] = record.month;
          obj["consumption"] = record.consumption;
        }
        String histMessage;
        size_t histBytes = serializeJson(histDoc, histMessage);
        if (histBytes > 0 && histMessage.length() > 0 && ESP.getFreeHeap() > 5000) {
          webSocket.sendTXT(num, histMessage);
        }
      }
      break;

    case WStype_TEXT:
      {
        Serial.println("[WS] Received: " + String((char*)payload));

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          String command = doc["command"];

          if (command == "pressButton") {
            int button = doc["button"];
            int duration = BUTTON_STANDARD_DURATION;

            if (button == 0) {
              duration = BUTTON_POWER_DURATION;
            }

            hardware.pressButton(button, duration);

          } else if (command == "getData") {
            SensorData data = hardware.getSensorData();
            EnergyData energyData = energyMonitor.getStableEnergyData();
            broadcastData(data, energyData);

          } else if (command == "setAutoPowerOn") {
            // Validate enabled parameter
            if (!doc.containsKey("enabled")) {
              StaticJsonDocument<128> error;
              error["type"] = "error";
              error["message"] = "Missing 'enabled' parameter";
              String out;
              serializeJson(error, out);
              webSocket.sendTXT(num, out);
              break;
            }
            
            bool enabled = doc["enabled"].as<bool>();
            hardware.setAutoPowerOn(enabled);
            Serial.println("[WS] Auto Power On set to: " + String(enabled ? "ENABLED" : "DISABLED"));

            StaticJsonDocument<128> response;
            response["autoPowerOn"] = enabled;
            String output;
            serializeJson(response, output);
            webSocket.broadcastTXT(output);

          } else if (command == "getAutoPowerOn") {
            StaticJsonDocument<128> response;
            response["autoPowerOn"] = hardware.getAutoPowerOn();
            String output;
            serializeJson(response, output);
            webSocket.sendTXT(num, output);

          } else if (command == "scanWifi") {
            int n = WiFi.scanNetworks();
            StaticJsonDocument<512> response;
            response["type"] = "wifiScanResult";
            JsonArray networks = response.createNestedArray("networks");
            for (int i = 0; i < n; ++i) {
              JsonObject net = networks.createNestedObject();
              net["ssid"] = WiFi.SSID(i);
              net["rssi"] = WiFi.RSSI(i);
              net["encryption"] = WiFi.encryptionType(i);
            }
            String out;
            serializeJson(response, out);
            webSocket.sendTXT(num, out);

          } else if (command == "setWifi") {
            // Validate input parameters
            if (!doc.containsKey("ssid")) {
              StaticJsonDocument<128> error;
              error["type"] = "error";
              error["message"] = "Missing 'ssid' parameter";
              String out;
              serializeJson(error, out);
              webSocket.sendTXT(num, out);
              break;
            }
            
            String ssid = doc["ssid"].as<String>();
            String password = doc.containsKey("password") ? doc["password"].as<String>() : "";
            
            // Validate SSID length (WiFi SSID max 32 chars)
            if (ssid.length() == 0 || ssid.length() > 32) {
              StaticJsonDocument<128> error;
              error["type"] = "error";
              error["message"] = "Invalid SSID length (1-32 characters)";
              String out;
              serializeJson(error, out);
              webSocket.sendTXT(num, out);
              break;
            }
            
            // Validate password length (WiFi password max 63 chars for WPA2)
            if (password.length() > 63) {
              StaticJsonDocument<128> error;
              error["type"] = "error";
              error["message"] = "Password too long (max 63 characters)";
              String out;
              serializeJson(error, out);
              webSocket.sendTXT(num, out);
              break;
            }

            if (ssid.length() > 0) {
              if (wifiMgr.setCredentials(ssid, password)) {
                wifiMgr.saveCredentials();
                StaticJsonDocument<256> resp;
                resp["type"] = "wifiStatus";
                resp["success"] = true;
                resp["message"] = "WiFi credentials updated. Rebooting...";
                String out;
                serializeJson(resp, out);
                webSocket.sendTXT(num, out);
                delay(1000);
                ESP.restart();
              }
            }

          } else if (command == "getCalibration") {
            bool getDefaults = doc["defaults"] | false;
            
            CalibrationData cal;
            
            if(getDefaults) {
              Serial.println("[WS] Loading DEFAULT calibration values");
              cal.sct013CalIn = SCT013_CALIBRATION_IN_DEFAULT;
              cal.sct013OffsetIn = SCT013_OFFSET_IN_DEFAULT;
              cal.sct013CalOut = SCT013_CALIBRATION_OUT_DEFAULT;
              cal.sct013OffsetOut = SCT013_OFFSET_OUT_DEFAULT;
              cal.batteryDividerRatio = BATTERY_DIVIDER_RATIO_DEFAULT;
              cal.batteryAdcCalibration = BATTERY_ADC_CALIBRATION_DEFAULT;
              cal.voltageOffsetCharge = VOLTAGE_OFFSET_CHARGE_DEFAULT;
              cal.voltageOffsetDischarge = VOLTAGE_OFFSET_DISCHARGE_DEFAULT;
              cal.voltageOffsetRest = VOLTAGE_OFFSET_REST_DEFAULT;
              cal.fixedVoltage = 0.0f;  // NEW default
              cal.mainsVoltage = MAINS_VOLTAGE; // NEW
            } else {
              cal = hardware.getCalibrationData();
            }
            
            StaticJsonDocument<896> resp;
            resp["type"] = "calibrationData";
            resp["sct013CalIn"] = cal.sct013CalIn;
            resp["sct013OffsetIn"] = cal.sct013OffsetIn;
            resp["sct013CalOut"] = cal.sct013CalOut;
            resp["sct013OffsetOut"] = cal.sct013OffsetOut;
            resp["batteryDividerRatio"] = cal.batteryDividerRatio;
            resp["batteryAdcCalibration"] = cal.batteryAdcCalibration;
            resp["voltageOffsetCharge"] = cal.voltageOffsetCharge;
            resp["voltageOffsetDischarge"] = cal.voltageOffsetDischarge;
            resp["voltageOffsetRest"] = cal.voltageOffsetRest;
            resp["fixedVoltage"] = cal.fixedVoltage;      // NEW
            resp["mainsVoltage"] = cal.mainsVoltage;      // NEW
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);

          } else if (command == "saveCalibration") {
            CalibrationData cal;
            
            // Validate and set calibration values with range checks
            float temp;
            
            temp = doc["sct013CalIn"] | SCT013_CALIBRATION_IN_DEFAULT;
            cal.sct013CalIn = (temp >= 1.0 && temp <= 100.0) ? temp : SCT013_CALIBRATION_IN_DEFAULT;
            
            temp = doc["sct013OffsetIn"] | SCT013_OFFSET_IN_DEFAULT;
            cal.sct013OffsetIn = (temp >= -5.0 && temp <= 5.0) ? temp : SCT013_OFFSET_IN_DEFAULT;
            cal.sct013OffsetIn = doc["sct013OffsetIn"] | SCT013_OFFSET_IN_DEFAULT;
            cal.sct013CalOut = doc["sct013CalOut"] | SCT013_CALIBRATION_OUT_DEFAULT;
            cal.sct013OffsetOut = doc["sct013OffsetOut"] | SCT013_OFFSET_OUT_DEFAULT;
            cal.batteryDividerRatio = doc["batteryDividerRatio"] | BATTERY_DIVIDER_RATIO_DEFAULT;
            cal.batteryAdcCalibration = doc["batteryAdcCalibration"] | BATTERY_ADC_CALIBRATION_DEFAULT;
            cal.voltageOffsetCharge = doc["voltageOffsetCharge"] | VOLTAGE_OFFSET_CHARGE_DEFAULT;
            cal.voltageOffsetDischarge = doc["voltageOffsetDischarge"] | VOLTAGE_OFFSET_DISCHARGE_DEFAULT;
            cal.voltageOffsetRest = doc["voltageOffsetRest"] | VOLTAGE_OFFSET_REST_DEFAULT;
            cal.fixedVoltage = doc["fixedVoltage"] | 0.0f;                  // NEW
            cal.mainsVoltage = doc["mainsVoltage"] | MAINS_VOLTAGE;         // NEW

            hardware.applyCalibration(cal);
            hardware.saveCalibration();

            StaticJsonDocument<256> resp;
            resp["type"] = "calibrationStatus";
            resp["success"] = true;
            resp["message"] = "Calibrazione salvata con successo";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);

          } else if (command == "getAdvancedSettings") {
            bool getDefaults = doc["defaults"] | false;
            
            AdvancedSettings adv;
            
            if(getDefaults) {
              Serial.println("[WS] Loading DEFAULT advanced settings");
              adv.powerStationOffVoltage = POWER_STATION_OFF_VOLTAGE_DEFAULT;
              adv.powerThreshold = POWER_THRESHOLD_DEFAULT;
              adv.powerFilterAlpha = POWER_FILTER_ALPHA_DEFAULT;
              adv.voltageMinSafe = VOLTAGE_MIN_SAFE_DEFAULT;
              adv.batteryLowWarning = BATTERY_LOW_WARNING_DEFAULT;
              adv.batteryCritical = BATTERY_CRITICAL_DEFAULT;
              adv.autoPowerOnDelay = AUTO_POWER_ON_DELAY_DEFAULT;
              adv.socBufferSize = SOC_BUFFER_SIZE_DEFAULT;
              adv.socChangeThreshold = SOC_CHANGE_THRESHOLD_DEFAULT;
              adv.warmupDelay = WARMUP_DELAY_DEFAULT;
            } else {
              adv = hardware.getAdvancedSettings();
            }
            
            StaticJsonDocument<512> resp;
            resp["type"] = "advancedSettings";
            resp["powerStationOffVoltage"] = adv.powerStationOffVoltage;
            resp["powerThreshold"] = adv.powerThreshold;
            resp["powerFilterAlpha"] = adv.powerFilterAlpha;
            resp["voltageMinSafe"] = adv.voltageMinSafe;
            resp["batteryLowWarning"] = adv.batteryLowWarning;
            resp["batteryCritical"] = adv.batteryCritical;
            resp["autoPowerOnDelay"] = adv.autoPowerOnDelay;
            resp["socBufferSize"] = adv.socBufferSize;
            resp["socChangeThreshold"] = adv.socChangeThreshold;
            resp["warmupDelay"] = adv.warmupDelay;
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);

          } else if (command == "saveAdvancedSettings") {
            Serial.println("[WS] Saving Advanced Settings");
            
            AdvancedSettings adv;
            adv.powerStationOffVoltage = doc["powerStationOffVoltage"] | POWER_STATION_OFF_VOLTAGE_DEFAULT;
            adv.powerThreshold = doc["powerThreshold"] | POWER_THRESHOLD_DEFAULT;
            adv.powerFilterAlpha = doc["powerFilterAlpha"] | POWER_FILTER_ALPHA_DEFAULT;
            adv.voltageMinSafe = doc["voltageMinSafe"] | VOLTAGE_MIN_SAFE_DEFAULT;
            adv.batteryLowWarning = doc["batteryLowWarning"] | BATTERY_LOW_WARNING_DEFAULT;
            adv.batteryCritical = doc["batteryCritical"] | BATTERY_CRITICAL_DEFAULT;
            adv.autoPowerOnDelay = doc["autoPowerOnDelay"] | AUTO_POWER_ON_DELAY_DEFAULT;
            adv.socBufferSize = doc["socBufferSize"] | SOC_BUFFER_SIZE_DEFAULT;
            adv.socChangeThreshold = doc["socChangeThreshold"] | SOC_CHANGE_THRESHOLD_DEFAULT;
            adv.warmupDelay = doc["warmupDelay"] | WARMUP_DELAY_DEFAULT;

            hardware.applyAdvancedSettings(adv);
            hardware.saveAdvancedSettings();

            StaticJsonDocument<256> resp;
            resp["type"] = "advancedSettingsStatus";
            resp["success"] = true;
            resp["message"] = "Impostazioni avanzate salvate con successo";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            
          } else if (command == "saveMqttConfig") {
            MQTTConfig config;
            config.enabled = doc["enabled"] | false;
            config.server = doc["server"].as<String>();
            config.port = doc["port"] | 1883;
            config.username = doc["username"].as<String>();
            config.password = doc["password"].as<String>();
            config.clientId = doc["clientId"].as<String>();
            
            mqttClient.setConfig(config);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "mqttStatus";
            resp["success"] = true;
            resp["message"] = "Configurazione MQTT salvata. Riavvio...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            delay(1000);
            ESP.restart();
            
          } else if (command == "saveHttpConfig") {
            HTTPConfig config;
            config.enabled = doc["enabled"] | false;
            config.server = doc["server"].as<String>();
            config.port = doc["port"] | 8123;
            config.endpoint = doc["endpoint"].as<String>();
            config.apiKey = doc["apiKey"].as<String>();
            
            httpClient.setConfig(config);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "httpStatus";
            resp["success"] = true;
            resp["message"] = "Configurazione Home Assistant salvata. Riavvio...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            delay(1000);
            ESP.restart();
            
          } else if (command == "saveUpsConfig") {
            UPSConfig config;
            config.enabled = doc["enabled"] | true;
            config.port = doc["port"] | 3493;
            config.shutdownThreshold = doc["shutdownThreshold"] | 50;
            
            upsProtocol.setConfig(config);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "upsStatus";
            resp["success"] = true;
            resp["message"] = "Configurazione UPS salvata. Riavvio...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            delay(1000);
            ESP.restart();
            
          } else if (command == "saveSystemSettings") {
            Serial.println("[WS] Saving System Settings");
            
            SystemSettings settings;
            settings.ntpServer = doc["ntpServer"].as<String>();
            settings.gmtOffset = doc["gmtOffset"] | NTP_GMT_OFFSET_DEFAULT;
            settings.daylightOffset = doc["daylightOffset"] | NTP_DAYLIGHT_OFFSET_DEFAULT;
            settings.beepsEnabled = doc["beepsEnabled"] | true;
            
            // Validate log level
            int newLogLevel = doc["logLevel"] | g_logLevel;
            if (newLogLevel >= LOG_LEVEL_DEBUG && newLogLevel <= LOG_LEVEL_NONE) {
              settings.logLevel = newLogLevel;
            } else {
              settings.logLevel = g_logLevel;
            }
            settings.valid = true;
            
            if (settings.ntpServer.length() == 0) {
              settings.ntpServer = NTP_SERVER_DEFAULT;
            }
            
            saveSystemSettingsToSPIFFS(settings);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "systemSettingsStatus";
            resp["success"] = true;
            resp["message"] = "Settings saved. Rebooting...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            delay(1000);
            ESP.restart();
            
          } else if (command == "saveHttpShutdownConfig") {
            Serial.println("[WS] Saving HTTP Shutdown Configuration");
            
            HttpShutdownConfig config;
            config.enabled = doc["enabled"] | false;
            config.batteryThreshold = doc["batteryThreshold"] | HTTP_SHUTDOWN_THRESHOLD_DEFAULT;
            config.server = doc["server"].as<String>();
            config.port = doc["port"] | HTTP_SHUTDOWN_PORT_DEFAULT;
            config.password = doc["password"].as<String>();
            config.shutdownSent = false;
            config.valid = true;
            
            saveHttpShutdownConfigToSPIFFS(config);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "httpShutdownStatus";
            resp["success"] = true;
            resp["message"] = "HTTP Shutdown configuration saved successfully";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            
          } else if (command == "saveApiPassword") {
            String password = doc["password"].as<String>();
            
            if (password.length() > 0) {
              saveAPIPasswordToSPIFFS(password);
              
              StaticJsonDocument<256> resp;
              resp["type"] = "apiPasswordStatus";
              resp["success"] = true;
              resp["message"] = "Password API salvata con successo";
              String out;
              serializeJson(resp, out);
              webSocket.sendTXT(num, out);
              
              Serial.println("[API] API password updated via WebSocket");
            } else {
              StaticJsonDocument<256> resp;
              resp["type"] = "apiPasswordStatus";
              resp["success"] = false;
              resp["message"] = "Password non valida";
              String out;
              serializeJson(resp, out);
              webSocket.sendTXT(num, out);
            }
            
          } else if (command == "resetMonthlyEnergy") {
            energyMonitor.resetMonthlyStats();
            
            StaticJsonDocument<256> resp;
            resp["type"] = "energyStatus";
            resp["success"] = true;
            resp["message"] = "Dati energetici mensili resettati";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            
          } else if (command == "factoryReset") {
            Serial.println("[WS] Factory reset requested");
            
            // Delete all configuration files
            SPIFFS.remove("/wifi.json");
            SPIFFS.remove("/calibration.json");
            SPIFFS.remove("/advanced.json");
            SPIFFS.remove("/autopoweron.txt");
            SPIFFS.remove("/mqtt_config.json");
            SPIFFS.remove("/ha_config.json");
            SPIFFS.remove("/ups_config.json");
            SPIFFS.remove("/energy_state.json");
            SPIFFS.remove("/energy_history.json");
            SPIFFS.remove(API_PASSWORD_FILE);
            SPIFFS.remove(SYSTEM_SETTINGS_FILE);
            SPIFFS.remove(HTTP_SHUTDOWN_CONFIG_FILE);
            
            StaticJsonDocument<256> resp;
            resp["type"] = "factoryResetStatus";
            resp["success"] = true;
            resp["message"] = "Reset di fabbrica completato. Riavvio...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            
            delay(2000);
            ESP.restart();
            
          } else if (command == "reboot") {
            Serial.println("[WS] Reboot requested");
            
            StaticJsonDocument<256> resp;
            resp["type"] = "rebootStatus";
            resp["success"] = true;
            resp["message"] = "Riavvio...";
            String out;
            serializeJson(resp, out);
            webSocket.sendTXT(num, out);
            
            delay(1000);
            ESP.restart();
          }
        }
      }
      break;

    default:
      break;
  }
}