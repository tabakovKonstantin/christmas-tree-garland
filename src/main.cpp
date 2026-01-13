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

// Reference globals
extern AsyncWebServer server; 
extern Config config;

void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n--- BOOTING ---");

  if (!FileManager::begin()) return;

  // --- DOUBLE RESET PROTECTION ---
  // Если файл флага существует при загрузке -> значит был сброс
  if (LittleFS.exists(RESET_FLAG_FILE)) {
    Serial.println("!!! Double Reset Detected: Erasing Config !!!");
    ledControl.showError();
    ConfigManager::eraseConfig();
    LittleFS.remove(RESET_FLAG_FILE);
    delay(1000);
    ESP.restart();
  } else {
    // Создаем флаг. Если в течение 3 сек нажать RESET, файл останется
    File f = LittleFS.open(RESET_FLAG_FILE, "w");
    if (f) f.close();
    
    Serial.println("Press RESET now to clear config...");
    delay(3000); // Окно 3 секунды для сброса настроек
    
    // Если пережили задержку, удаляем флаг
    LittleFS.remove(RESET_FLAG_FILE);
    Serial.println("Booting normally.");
  }
  // --------------------------------

  ledControl.initLEDs();
  
  // Пытаемся подключиться. Если конфига нет -> запускается AP (Точка доступа)
  initWiFi(); 

  // Если подключились к роутеру (STA) или работаем как AP+STA
  if (WiFi.status() == WL_CONNECTED || WiFi.getMode() == WIFI_AP) {
    ConfigManager::loadConfig(config);
    
    // mDNS (доступ по http://garland-XXXX.local)
    String dnsName = "garland-" + String(ESP.getChipId(), HEX);
    if (MDNS.begin(dnsName.c_str())) {
        Serial.printf("mDNS started: http://%s.local\n", dnsName.c_str());
        MDNS.addService("http", "tcp", 80);
    }

    OtaManager::setup();
    
    // НАСТРОЙКА ВЕБ-ИНТЕРФЕЙСА
    webControl.setup(server);
    
    // ВАЖНО: Запускаем сервер. 
    // В режиме AP он уже запущен в initWiFi, но повторный вызов безопасен.
    // В режиме STA (подключено к роутеру) это критически важно!
    server.begin(); 
    Serial.println("Web Server Started");
    
    // Запуск MQTT только если есть интернет и он включен
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