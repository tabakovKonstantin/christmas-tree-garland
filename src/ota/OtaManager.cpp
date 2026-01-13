#include "OtaManager.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

void OtaManager::setup() {
    String hostname = "garland-" + String(ESP.getChipId(), HEX);
    ArduinoOTA.setHostname(hostname.c_str());

    Serial.println("=== OTA Debug ===");
    Serial.print("Hostname: "); Serial.println(hostname);
    if (WiFi.getMode() == WIFI_STA) {
        Serial.print("STA IP: "); Serial.println(WiFi.localIP());
    } else {
        Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    }

    ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nOTA End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]\n", error);
    });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}