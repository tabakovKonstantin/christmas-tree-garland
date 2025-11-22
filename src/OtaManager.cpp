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

    // Extra debug: print network info BEFORE OTA starts
    Serial.println("=== OTA Network Debug Info ===");
    Serial.print("WiFi mode: ");
    Serial.println(WiFi.getMode() == WIFI_STA ? "STA" : "AP/OTHER");

    Serial.print("Device IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    Serial.println("Hostname assigned: " + hostname);
    Serial.println("==============================");

    // Optional password:
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
        unsigned int percent = (total > 0) ? (progress * 100U) / total : 0;
        Serial.printf("OTA progress: %u%%\r", percent);
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("OTA error[%u]: ", error);

        // New: Verbose error decoding
        switch (error)
        {
        case OTA_AUTH_ERROR:
            Serial.println("AUTH ERROR (wrong password?)");
            break;

        case OTA_BEGIN_ERROR:
            Serial.println("BEGIN ERROR (flash init failed)");
            break;

        case OTA_CONNECT_ERROR:
            Serial.println("CONNECT ERROR (ESP could not open TCP connection to host)");
            Serial.println("Potential reasons:");
            Serial.println(" - Wrong host_ip in platformio.ini upload_flags");
            Serial.println(" - Firewall/VPN on host blocks incoming TCP");
            Serial.println(" - Host and ESP not in same network segment");
            Serial.println(" - Host chosen wrong interface (0.0.0.0 detected)");
            break;

        case OTA_RECEIVE_ERROR:
            Serial.println("RECEIVE ERROR (transfer interrupted)");
            Serial.println("Possible reasons:");
            Serial.println(" - Weak WiFi signal (check RSSI)");
            Serial.println(" - Packet loss / router filtering UDP/TCP");
            break;

        case OTA_END_ERROR:
            Serial.println("END ERROR (connection closed prematurely)");
            Serial.println("Often follows CONNECT/RECEIVE issues.");
            Serial.println("Check above logs for root cause.");
            break;

        default:
            Serial.println("Unknown OTA error");
            break;
        }

        // Additional debug printout of current network state:
        Serial.println();
        Serial.println("=== Live Network State at Error ===");
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
        Serial.print("Local IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("Subnet: ");
        Serial.println(WiFi.subnetMask());
        Serial.print("RSSI: ");
        Serial.println(WiFi.RSSI());
        Serial.println("===================================");
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
    ArduinoOTA.handle();
}