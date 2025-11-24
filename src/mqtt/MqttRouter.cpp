#include "MqttRouter.h"

MqttRouter::MqttRouter() : handlerCount(0)
{
    for (uint8_t i = 0; i < MAX_HANDLERS; ++i)
    {
        handlers[i] = nullptr;
    }
}

bool MqttRouter::addHandler(MqttMessageHandler *handler)
{
    if (handlerCount >= MAX_HANDLERS || handler == nullptr)
    {
        return false;
    }
    handlers[handlerCount++] = handler;
    return true;
}

void MqttRouter::route(const String &topic, const String &payload)
{
    for (uint8_t i = 0; i < handlerCount; ++i)
    {
        MqttMessageHandler *h = handlers[i];
        if (!h)
        {
            continue;
        }

        if (h->canHandle(topic))
        {
            h->handleMessage(topic, payload);
            return;
        }
    }

    // No handler found, just log and ignore.
    Serial.println("  [MQTT] No handler registered for topic, ignoring.");
}