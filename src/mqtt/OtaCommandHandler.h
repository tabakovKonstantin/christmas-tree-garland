#ifndef OTA_COMMAND_HANDLER_H
#define OTA_COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "MqttMessageHandler.h"
#include "../LedControl.h"
#include "../ota/HttpOtaUpdater.h"

// Handles OTA control messages on a specific MQTT topic.
// Topic pattern (from MqttManager): home/lights/<id>/ota
// Expected payload JSON example:
//   {"command": "update", "url": "http://your-server.com/firmware.bin"}
class OtaCommandHandler : public MqttMessageHandler
{
public:
    explicit OtaCommandHandler(LedControl &ledControlRef);

    // Set concrete topic this handler should serve.
    void setTopic(const String &topic);

    bool canHandle(const String &topic) const override;
    void handleMessage(const String &topic, const String &payload) override;

private:
    LedControl &ledControl;
    String otaTopic;
};

#endif