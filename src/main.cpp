#include <Arduino.h>
#include <ESP8266mDNS.h>
#include "ConnectManager.h"
#include "FileManager.h"
#include "ConfigManager.h"
#include "MqttManager.h"
#include "LedControl.h"
#include "WebControlManager.h"
#include "ota/OtaManager.h"

#define RESET_FLAG_FILE "/reset.flag"

EffectManager effectManager;
LedControl ledControl(effectManager);
MqttManager mqttManager(ledControl);
WebControlManager webControl(ledControl);

extern AsyncWebServer server; 
extern Config config;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n--- BOOTING ---");

  if (!FileManager::begin()) {
      Serial.println("FS Error, halting.");
      return;
  }

  // --- STANDARD DOUBLE RESET DETECTOR ---
  if (LittleFS.exists(RESET_FLAG_FILE)) {
    Serial.println("\n!!! DOUBLE RESET DETECTED !!!");
    ledControl.showError();
    ConfigManager::eraseConfig();
    LittleFS.remove(RESET_FLAG_FILE);
    delay(1000);
    ESP.restart();
  } else {
    // Create flag
    File f = LittleFS.open(RESET_FLAG_FILE, "w");
    if (f) f.close();
    
    // Simple 3s delay window. If reset happens here, file remains.
    // If not, we remove it.
    delay(3000); 
    
    LittleFS.remove(RESET_FLAG_FILE);
  }
  // --------------------------------

  ledControl.initLEDs();
  initWiFi(); 

  // Services
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    ConfigManager::loadConfig(config);
    
    String dnsName = "garland-" + String(ESP.getChipId(), HEX);
    if (MDNS.begin(dnsName.c_str())) {
        Serial.printf("[mDNS] Started: http://%s.local\n", dnsName.c_str());
        MDNS.addService("http", "tcp", 80);
    }

    OtaManager::setup();
    
    // Determine which services to start based on workMode
    // 0 = Web Only
    // 1 = MQTT Only
    // 2 = Both
    
    bool enableWebControl = (config.workMode == 0 || config.workMode == 2);
    bool enableMqtt = (config.workMode == 1 || config.workMode == 2);

    // If Mode 1 (MQTT Only), setup web server with restricted UI (just reset link)
    // so user can still factory reset via browser if needed.
    webControl.setup(server, enableWebControl);
    
    server.begin(); 
    Serial.println("[HTTP] Server Started");
    
    if (WiFi.status() == WL_CONNECTED && enableMqtt) {
      mqttManager.init();
    }
  }
}

void loop()
{
  OtaManager::handle();
  MDNS.update();
  FastLED.show();
}