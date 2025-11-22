#ifndef MQTT_MESSAGE_HANDLER_H
#define MQTT_MESSAGE_HANDLER_H

#include <Arduino.h>

// Base interface for MQTT message handlers.
// Each handler decides whether it can process a given topic (canHandle)
// and then performs the logic in handleMessage.
class MqttMessageHandler
{
public:
    virtual ~MqttMessageHandler() {}

    // Return true if this handler is responsible for this topic.
    virtual bool canHandle(const String &topic) const = 0;

    // Handle the message for the given topic. Payload is the raw message.
    virtual void handleMessage(const String &topic, const String &payload) = 0;
};

#endif