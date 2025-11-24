#include <Arduino.h>
#include "ConnectManager.h"
#include "FileManager.h"
#include "ConfigManager.h"
#include "MqttManager.h"
#include "LedControl.h"
#include "ota/OtaManager.h"
#include <ESP8266WiFi.h>

#define RESET_FLAG_FILE "/reset.flag"
#define DOUBLE_RESET_TIMEOUT 5000

EffectManager effectManager;
LedControl ledControl(effectManager);
MqttManager mqttManager(ledControl);

void checkDoubleReset()
{
  if (LittleFS.exists(RESET_FLAG_FILE))
  {
    Serial.println("Double reset detected. Erasing configuration...");
    ConfigManager::eraseConfig();
    LittleFS.remove(RESET_FLAG_FILE);
    delay(1000);
    ESP.restart();
  }
  else
  {
    File flag = LittleFS.open(RESET_FLAG_FILE, "w");
    if (flag)
    {
      flag.close();
      Serial.println("First reset detected. Waiting for second reset...");
      delay(DOUBLE_RESET_TIMEOUT);
      LittleFS.remove(RESET_FLAG_FILE);
      Serial.println("No second reset detected.");
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting device...");

  if (!FileManager::begin())
  {
    Serial.println("File system initialization failed");
    return;
  }

  checkDoubleReset();

  ledControl.initLEDs();

  initWiFi();

  if (WiFi.getMode() == WIFI_STA && WiFi.isConnected())
  {
    Serial.println("Wi-Fi connected in STA mode, enabling OTA and MQTT...");
    OtaManager::setup();
    mqttManager.init();
  }
  else
  {
    Serial.println("Wi-Fi not connected in STA mode. OTA and MQTT are not started.");
  }
}

void loop()
{
  OtaManager::handle();
  FastLED.show();
}