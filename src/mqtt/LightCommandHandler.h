#ifndef LIGHT_COMMAND_HANDLER_H
#define LIGHT_COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "MqttMessageHandler.h"
#include "../Payload.h"
#include "../LedControl.h"

// Handles light command messages on a specific MQTT topic.
// Topic pattern (from MqttManager): home/lights/<id>/set
class LightCommandHandler : public MqttMessageHandler
{
public:
    explicit LightCommandHandler(LedControl &ledControlRef);

    // Set concrete topic this handler should serve.
    void setTopic(const String &topic);

    bool canHandle(const String &topic) const override;
    void handleMessage(const String &topic, const String &payload) override;

private:
    LedControl &ledControl;
    String commandTopic;
};

#endif