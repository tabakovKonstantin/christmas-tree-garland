#ifndef HTTP_OTA_UPDATER_H
#define HTTP_OTA_UPDATER_H

#include <Arduino.h>

// Simple HTTP-based OTA updater for ESP8266.
// Triggered via MQTT: firmware URL comes from the MQTT message payload.
// This is intended for remote devices (friends' garlands) where you cannot
// use local ArduinoOTA/esptool directly.
class HttpOtaUpdater
{
public:
    // Start OTA update from the given HTTP/HTTPS URL.
    // Example URL: "http://your-server.com/garland/firmware.bin"
    //
    // Notes:
    //  - This function is blocking while downloading and flashing.
    //  - On success, ESP will reboot automatically.
    //  - On failure, it prints detailed logs and returns to normal operation.
    static void updateFromUrl(const String &url);
};

#endif