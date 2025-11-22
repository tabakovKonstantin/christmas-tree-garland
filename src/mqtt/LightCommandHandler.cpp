#include "LightCommandHandler.h"

LightCommandHandler::LightCommandHandler(LedControl &ledControlRef)
    : ledControl(ledControlRef)
{
}

void LightCommandHandler::setTopic(const String &topic)
{
    commandTopic = topic;
}

bool LightCommandHandler::canHandle(const String &topic) const
{
    return topic == commandTopic;
}

void LightCommandHandler::handleMessage(const String &topic, const String &payload)
{
    Serial.println("  [LightHandler] Handling light command message.");

    Payload incomingPayload;
    if (incomingPayload.fromJson(payload))
    {
        Serial.print("  [LightHandler] Parsed JSON: ");
        Serial.println(incomingPayload.toJson());
        ledControl.changeState(incomingPayload);
        return;
    }

    Serial.println("  [LightHandler] Failed to parse JSON message for light command.");
}