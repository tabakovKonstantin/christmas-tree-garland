#include "OtaCommandHandler.h"

OtaCommandHandler::OtaCommandHandler(LedControl &ledControlRef)
    : ledControl(ledControlRef)
{
}

void OtaCommandHandler::setTopic(const String &topic)
{
    otaTopic = topic;
}

bool OtaCommandHandler::canHandle(const String &topic) const
{
    return topic == otaTopic;
}

void OtaCommandHandler::handleMessage(const String &topic, const String &payload)
{
    Serial.println("  [OtaHandler] Handling OTA control message.");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("  [OtaHandler] JSON parse error: ");
        Serial.println(error.c_str());
        return;
    }

    String command = doc["command"].as<String>();
    if (command.length() == 0)
    {
        command = doc["action"].as<String>();
    }

    String url = doc["url"].as<String>();

    Serial.print("  [OtaHandler] command: ");
    Serial.println(command);
    Serial.print("  [OtaHandler] url: ");
    Serial.println(url);

    if (!command.equalsIgnoreCase("update"))
    {
        Serial.println("  [OtaHandler] Command is not 'update', skipping OTA.");
        return;
    }

    if (url.length() == 0)
    {
        Serial.println("  [OtaHandler] URL is empty, cannot perform OTA.");
        return;
    }

    if (!url.startsWith("http://") && !url.startsWith("https://"))
    {
        Serial.println("  [OtaHandler] URL must start with http:// or https://, skipping.");
        return;
    }

    ledControl.showSuccess();
    Serial.println("  [OtaHandler] Starting HTTP OTA update from URL...");
    HttpOtaUpdater::updateFromUrl(url);
}