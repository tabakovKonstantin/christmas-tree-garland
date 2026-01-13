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

// Reference globals defined in ConnectManager.cpp
extern AsyncWebServer server; 
extern Config config;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n--- BOOTING ---");

  if (!FileManager::begin()) {
      Serial.println("FS Init Failed!");
      return;
  }

  // --- DOUBLE RESET DETECTOR (BLOCKING & SAFE) ---
  if (LittleFS.exists(RESET_FLAG_FILE)) {
    Serial.println("!!! Double Reset Detected: Erasing Config !!!");
    ledControl.showError(); // Visual feedback
    ConfigManager::eraseConfig();
    LittleFS.remove(RESET_FLAG_FILE);
    delay(1000);
    ESP.restart();
  } else {
    // Create flag to detect next reset
    File f = LittleFS.open(RESET_FLAG_FILE, "w");
    if (f) f.close();
    
    Serial.println("Waiting for double reset...");
    delay(3000); // 3 seconds window to press reset
    
    // Remove flag if we survived the delay
    LittleFS.remove(RESET_FLAG_FILE);
    Serial.println("Booting normally.");
  }
  // -----------------------------------------------

  ledControl.initLEDs();
  initWiFi(); 

  // Start Services if network is available (STA or AP)
  if (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    // Reload config into the global variable used by other modules
    ConfigManager::loadConfig(config);
    
    String dnsName = "garland-" + String(ESP.getChipId(), HEX);
    if (MDNS.begin(dnsName.c_str())) {
        Serial.printf("mDNS started: http://%s.local\n", dnsName.c_str());
        MDNS.addService("http", "tcp", 80);
    }

    OtaManager::setup();
    webControl.setup(server);
    
    if (WiFi.status() == WL_CONNECTED && config.mqttEnabled) {
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