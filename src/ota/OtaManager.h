#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

// Simple wrapper around ArduinoOTA for ESP8266.
// Call OtaManager::setup() once after Wi-Fi is connected in STA mode.
// Call OtaManager::handle() regularly in loop().

class OtaManager
{
public:
    // Initialize ArduinoOTA (hostname, callbacks, etc.)
    static void setup();

    // Process incoming OTA requests.
    static void handle();
};

#endif