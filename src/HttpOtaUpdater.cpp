#include "HttpOtaUpdater.h"

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266httpUpdate.h>

void HttpOtaUpdater::updateFromUrl(const String &url)
{
    Serial.println();
    Serial.println("========== MQTT HTTP OTA REQUEST ==========");
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[HTTP-OTA] Wi-Fi is not connected, cannot start update.");
        Serial.println("===========================================");
        return;
    }

    if (!url.startsWith("http://") && !url.startsWith("https://"))
    {
        Serial.print("[HTTP-OTA] Invalid URL (must start with http:// or https://): ");
        Serial.println(url);
        Serial.println("===========================================");
        return;
    }

    Serial.print("[HTTP-OTA] Starting HTTP OTA from URL: ");
    Serial.println(url);

    Serial.print("[HTTP-OTA] Current IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("[HTTP-OTA] Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("[HTTP-OTA] RSSI: ");
    Serial.println(WiFi.RSSI());

    // Important: this call is blocking. During update, normal loop logic is paused.
    // On success, ESPhttpUpdate will force a reboot.

    // Use explicit WiFiClient-based overload to match current ESP8266HTTPUpdate API.
    WiFiClient client;
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        Serial.println("[HTTP-OTA] Update FAILED");
        Serial.print("[HTTP-OTA] Error code: ");
        Serial.println(ESPhttpUpdate.getLastError());
        Serial.print("[HTTP-OTA] Error message: ");
        Serial.println(ESPhttpUpdate.getLastErrorString());
        Serial.println("[HTTP-OTA] Device will continue with existing firmware.");
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[HTTP-OTA] No updates available (HTTP 304 / no new firmware).");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("[HTTP-OTA] Update OK, device will reboot now.");
        // Normally not reached, because ESPhttpUpdate triggers restart.
        break;

    default:
        Serial.print("[HTTP-OTA] Unexpected return code: ");
        Serial.println(static_cast<int>(ret));
        break;
    }

    Serial.println("===========================================");
}