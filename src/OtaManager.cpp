#include "OtaManager.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

void OtaManager::setup()
{
    Serial.println();
    Serial.println("Initializing OTA...");

    // Generate a unique hostname based on the chip ID.
    String hostname = "garland-";
    hostname += String(ESP.getChipId(), HEX);
    hostname.toLowerCase();
    ArduinoOTA.setHostname(hostname.c_str());

    // Optional: set password here if you want OTA authentication.
    // ArduinoOTA.setPassword("your-ota-password");

    ArduinoOTA.onStart([]()
    {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else
        {
            type = "filesystem";
        }
        Serial.println("OTA start updating: " + type);
    });

    ArduinoOTA.onEnd([]()
    {
        Serial.println();
        Serial.println("OTA update finished");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        unsigned int percent = 0;
        if (total != 0)
        {
            percent = (progress * 100U) / total;
        }
        Serial.printf("OTA progress: %u%%\r", percent);
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("OTA error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            Serial.println("Auth failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Serial.println("Begin failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Serial.println("Connect failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Serial.println("Receive failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Serial.println("End failed");
        }
    });

    ArduinoOTA.begin();

    Serial.println("OTA is ready");
    Serial.print("Hostname: ");
    Serial.println(hostname);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void OtaManager::handle()
{
    // Handle OTA events (must be called often in loop()).
    ArduinoOTA.handle();
}