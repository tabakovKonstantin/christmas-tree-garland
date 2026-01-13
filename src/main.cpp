#include <Arduino.h>
#include <ESP8266mDNS.h>
#include "ConnectManager.h"
#include "FileManager.h"
#include "ConfigManager.h"
#include "MqttManager.h"
#include "LedControl.h"
#include "WebControlManager.h"
#include "ota/OtaManager.h"

EffectManager effectManager;
LedControl ledControl(effectManager);
MqttManager mqttManager(ledControl);
WebControlManager webControl(ledControl);
extern AsyncWebServer server; // From ConnectManager
Config globalConfig;

void setup()
{
  Serial.begin(115200);
  FileManager::begin();
  
  ledControl.initLEDs();
  initWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    ConfigManager::loadConfig(globalConfig);
    
    // Start mDNS
    String dnsName = "garland-" + String(ESP.getChipId(), HEX);
    if (MDNS.begin(dnsName.c_str())) {
        Serial.printf("mDNS started: http://%s.local\n", dnsName.c_str());
    }

    OtaManager::setup();
    
    // Web UI is always available as a fallback
    webControl.setup(server);
    
    if (globalConfig.mqttEnabled) {
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