#ifndef MQTT_ROUTER_H
#define MQTT_ROUTER_H

#include <Arduino.h>
#include "MqttMessageHandler.h"

// Lightweight router which keeps a static array of handler pointers.
// It finds the first handler whose canHandle() returns true and calls it.
class MqttRouter
{
public:
    static const uint8_t MAX_HANDLERS = 4;

    MqttRouter();

    // Register a handler. Returns true if added, false if list is full.
    bool addHandler(MqttMessageHandler *handler);

    // Route message to the first matching handler.
    void route(const String &topic, const String &payload);

private:
    MqttMessageHandler *handlers[MAX_HANDLERS];
    uint8_t handlerCount;
};

#endif