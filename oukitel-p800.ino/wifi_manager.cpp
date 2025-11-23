/*
 * WiFi Manager Implementation
 * Non-blocking State Machine
 */

#include "wifi_manager.h"
#include "logger.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

WiFiManager::WiFiManager() {
  currentState = WIFI_STATE_IDLE;
  retryCount = 0;
  lastStateChange = 0;
  apStartTime = 0;
  lastApClientCheck = 0;
  hasDefaultCredentials = false;
  
  credentials.ssid = "";
  credentials.password = "";
  credentials.valid = false;
}

bool WiFiManager::begin() {
  Serial.println("[WIFI] Initializing WiFi manager...");
  
  // Set hostname
  WiFi.setHostname("OUKITEL-P800");
  
  // Load saved credentials from SPIFFS
  loadCredentials();
  
  // If no saved credentials, try using defaults from config.h
  #ifdef DEFAULT_WIFI_SSID
  if (!credentials.valid) {
    Serial.println("[WIFI] No saved credentials found");
    Serial.println("[WIFI] Using default credentials from config.h");
    credentials.ssid = DEFAULT_WIFI_SSID;
    credentials.password = DEFAULT_WIFI_PASSWORD;
    credentials.valid = true;
    hasDefaultCredentials = true;
  }
  #endif
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  
  // Start connection process if we have credentials
  if (credentials.valid) {
    Serial.println("[WIFI] Starting connection to: " + credentials.ssid);
    WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
    currentState = WIFI_STATE_CONNECTING;
    lastStateChange = millis();
    retryCount = 0;
  } else {
    Serial.println("[WIFI] No credentials, starting AP mode");
    startAccessPoint();
  }
  
  return true;
}

void WiFiManager::handleConnection() {
  unsigned long now = millis();

  switch (currentState) {
    case WIFI_STATE_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        currentState = WIFI_STATE_CONNECTED;
        Serial.println("\n[WIFI] Connected successfully!");
        printNetworkInfo();
        retryCount = 0;
      } else if (now - lastStateChange > 10000) { // 10s timeout per attempt
        Serial.println("\n[WIFI] Connection attempt failed");
        retryCount++;
        if (retryCount >= 3) {
          Serial.println("[WIFI] All attempts failed. Starting AP mode.");
          startAccessPoint();
        } else {
          currentState = WIFI_STATE_WAITING_RETRY;
          lastStateChange = now;
          Serial.println("[WIFI] Waiting 5s before retry...");
        }
      }
      break;

    case WIFI_STATE_WAITING_RETRY:
      if (now - lastStateChange > 5000) { // 5s wait between retries
        currentState = WIFI_STATE_CONNECTING;
        Serial.println("[WIFI] Retrying connection (Attempt " + String(retryCount + 1) + ")...");
        WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
        lastStateChange = now;
      }
      break;

    case WIFI_STATE_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Connection lost!");
        currentState = WIFI_STATE_CONNECTING;
        WiFi.disconnect();
        WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
        lastStateChange = now;
        retryCount = 0; 
      }
      break;

    case WIFI_STATE_AP_MODE:
      dnsServer.processNextRequest();
      
      // Check for AP clients and timeout
      if (now - lastApClientCheck > 30000) {  // Check every 30 seconds
        lastApClientCheck = now;
        int clientCount = WiFi.softAPgetStationNum();
        
        if (clientCount == 0) {
          // No clients connected - check if timeout passed
          if (now - apStartTime > WIFI_AP_TIMEOUT) {
            Serial.println("[WIFI] No AP clients for timeout period - retrying WiFi connection");
            stopAccessPoint();
            currentState = WIFI_STATE_CONNECTING;
            retryCount = 0;
            WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
            lastStateChange = now;
          }
        } else {
          // Clients connected - reset timer
          apStartTime = now;
        }
      }
      break;
      
    default:
      break;
  }
}

void WiFiManager::startAccessPoint() {
  if (currentState == WIFI_STATE_AP_MODE) return;
  
  Serial.println("[WIFI] Free heap before AP: " + String(ESP.getFreeHeap()));
  
  // Stop WiFi first to clean up
  WiFi.disconnect(true);
  delay(100);
  
  // Set AP mode
  WiFi.mode(WIFI_AP);
  WiFi.setHostname("OUKITEL-P800");
  delay(500);
  
  // Start AP with explicit parameters
  Serial.println("[WIFI] Starting AP: " + String(AP_SSID));
  
  bool success;
  if (strlen(AP_PASSWORD) == 0) {
    Serial.println("[WIFI] Starting open network (no password)");
    success = WiFi.softAP(AP_SSID);
  } else {
    success = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, false, AP_MAX_CONNECTIONS);
  }
  
  if (success) {
    delay(500);
    Serial.println("[WIFI] AP started successfully");
    Serial.println("[WIFI] SSID: " + String(AP_SSID));
    Serial.println("[WIFI] IP: " + WiFi.softAPIP().toString());
    Serial.println("[WIFI] Connect to this network and go to: http://" + WiFi.softAPIP().toString());
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    currentState = WIFI_STATE_AP_MODE;
    apStartTime = millis();
    lastApClientCheck = millis();
    retryCount = 0;
  } else {
    Serial.println("[WIFI] ERROR: Failed to start AP");
    // Retry once
    delay(100);
    success = WiFi.softAP(AP_SSID);
    if (success) {
      Serial.println("[WIFI] AP started on retry");
      dnsServer.start(53, "*", WiFi.softAPIP());
      currentState = WIFI_STATE_AP_MODE;
      apStartTime = millis();
      lastApClientCheck = millis();
    } else {
      Serial.println("[WIFI] CRITICAL: Cannot start AP mode");
    }
  }
}

void WiFiManager::stopAccessPoint() {
  if (currentState != WIFI_STATE_AP_MODE) return;
  
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("OUKITEL-P800");
  
  currentState = WIFI_STATE_IDLE;
  Serial.println("[WIFI] AP mode stopped");
}

bool WiFiManager::setCredentials(const String& ssid, const String& password) {
  if (ssid.length() == 0) {
    return false;
  }
  
  credentials.ssid = ssid;
  credentials.password = password;
  credentials.valid = true;
  hasDefaultCredentials = false;
  
  // Save to SPIFFS
  saveCredentials();
  
  Serial.println("[WIFI] New credentials saved: " + ssid);
  Serial.println("[WIFI] Restarting connection...");
  
  // If we're in AP mode, stop it
  if (currentState == WIFI_STATE_AP_MODE) {
    stopAccessPoint();
  }
  
  // Restart WiFi connection
  WiFi.disconnect();
  currentState = WIFI_STATE_CONNECTING;
  retryCount = 0;
  WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());
  lastStateChange = millis();
  
  return true;
}

WiFiCredentials WiFiManager::getCredentials() {
  return credentials;
}

void WiFiManager::clearCredentials() {
  credentials.ssid = "";
  credentials.password = "";
  credentials.valid = false;
  hasDefaultCredentials = false;
  
  saveCredentials();
  
  Serial.println("[WIFI] Credentials cleared");
}

void WiFiManager::loadCredentials() {
  if (!SPIFFS.exists(WIFI_CREDS_FILE)) {
    return;
  }
  
  File file = SPIFFS.open(WIFI_CREDS_FILE, "r");
  if (!file) {
    Serial.println("[WIFI] Failed to open credentials file");
    return;
  }
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("[WIFI] Failed to parse credentials file");
    return;
  }
  
  credentials.ssid = doc["ssid"].as<String>();
  credentials.password = doc["password"].as<String>();
  credentials.valid = !credentials.ssid.isEmpty();
  
  if (credentials.valid) {
    Serial.println("[WIFI] Loaded saved credentials for: " + credentials.ssid);
    hasDefaultCredentials = false;
  }
}

void WiFiManager::saveCredentials() {
  DynamicJsonDocument doc(512);
  doc["ssid"] = credentials.ssid;
  doc["password"] = credentials.password;
  
  File file = SPIFFS.open(WIFI_CREDS_FILE, "w");
  if (!file) {
    Serial.println("[WIFI] Failed to save credentials");
    return;
  }
  
  serializeJson(doc, file);
  file.close();
  
  Serial.println("[WIFI] Credentials saved to SPIFFS");
}

bool WiFiManager::isConnected() {
  return (currentState == WIFI_STATE_CONNECTED);
}

bool WiFiManager::isAPMode() {
  return (currentState == WIFI_STATE_AP_MODE);
}

String WiFiManager::getLocalIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

String WiFiManager::getAPIP() {
  if (isAPMode()) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

String WiFiManager::getSSID() {
  if (isConnected()) {
    return WiFi.SSID();
  } else if (isAPMode()) {
    return String(AP_SSID);
  }
  return "Not connected";
}

int WiFiManager::getRSSI() {
  if (isConnected()) {
    return WiFi.RSSI();
  }
  return 0;
}

String WiFiManager::getMACAddress() {
  return WiFi.macAddress();
}

String WiFiManager::getConnectionStatus() {
  if (isAPMode()) {
    int clients = WiFi.softAPgetStationNum();
    unsigned long timeInAP = (millis() - apStartTime) / 1000;
    return "AP Mode - " + String(AP_SSID) + " (" + String(clients) + " clients, " + String(timeInAP) + "s)";
  } else if (currentState == WIFI_STATE_CONNECTING) {
    return "Connecting to " + credentials.ssid + "...";
  } else if (currentState == WIFI_STATE_WAITING_RETRY) {
    return "Waiting to retry connection...";
  } else if (isConnected()) {
    return "Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
  } else {
    return "Disconnected";
  }
}

void WiFiManager::printNetworkInfo() {
  Serial.println("[WIFI] Network Information:");
  Serial.println("  Status: " + getConnectionStatus());
  Serial.println("  IP Address: " + getLocalIP());
  Serial.println("  MAC Address: " + getMACAddress());
  Serial.println("  Hostname: OUKITEL-P800");
  if (isConnected()) {
    Serial.println("  RSSI: " + String(getRSSI()) + " dBm");
    Serial.println("  Gateway: " + WiFi.gatewayIP().toString());
    Serial.println("  DNS: " + WiFi.dnsIP().toString());
  }
}