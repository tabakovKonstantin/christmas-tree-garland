#include <Arduino.h>
#include "ConnectManager.h"
#include "FileManager.h"
#include "ConfigManager.h"
#include "MqttManager.h"
#include "LedControl.h"
#include "OtaManager.h"
#include <ESP8266WiFi.h>

#define RESET_FLAG_FILE "/reset.flag"
#define DOUBLE_RESET_TIMEOUT 5000

// Global managers used across the project.
EffectManager effectManager;
LedControl ledControl(effectManager);
MqttManager mqttManager(ledControl);

// Helper to detect a double reset within a short time window.
// If a double reset is detected, stored configuration is erased.
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

  // Initialize LittleFS via FileManager.
  if (!FileManager::begin())
  {
    Serial.println("File system initialization failed");
    return;
  }

  // Check for double reset to optionally wipe configuration.
  checkDoubleReset();

  // Initialize LED strip and show short notification.
  ledControl.initLEDs();

  // Initialize Wi-Fi (either connect in STA mode using stored config
  // or start an access point with configuration portal).
  initWiFi();

  // If device is in STA mode and connected to Wi-Fi:
  // - enable OTA updates
  // - initialize MQTT manager
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
  // Handle OTA update requests. This must be called often.
  OtaManager::handle();

  // Render LEDs. Effects use timers and update the buffer,
  // FastLED.show() sends the current buffer to the strip.
  FastLED.show();
}