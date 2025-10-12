#include "MqttManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <Ticker.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

extern LedControl ledControl;

MqttManager::MqttManager(LedControl &led_Control) : ledControl(led_Control) {}

void MqttManager::init()
{
    Serial.println();
    Serial.println("Initializing MQTT Manager...");

    mqttClient.onConnect([this](bool sessionPresent)
                         { this->onMqttConnect(sessionPresent); });
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason)
                            { this->onMqttDisconnect(reason); });
    mqttClient.onMessage([this](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
                         { this->onMqttMessage(topic, payload, properties, len, index, total); });

    IPAddress mqttIP;
    ConfigManager::loadConfig(config);
    if (mqttIP.fromString(config.mqttServer.c_str()))
    {
        mqttClient.setServer(mqttIP, config.mqttPort);
    }
    else
    {
        Serial.println("Invalid IP address format.");
        Serial.println(config.mqttServer);
        ledControl.showError();
        return;
    }

    if (config.mqttUsername.length() > 0 && config.mqttPassword.length() > 0)
    {
        mqttClient.setCredentials(config.mqttUsername.c_str(), config.mqttPassword.c_str());
        Serial.println("MQTT credentials set.");
    }
    else
    {
        Serial.println("No MQTT credentials provided, connecting without authentication.");
    }

    connectToMqtt();
}

void MqttManager::connectToMqtt()
{
    Serial.print("Connecting to MQTT broker ");
    Serial.print(config.mqttServer);
    Serial.print(":");
    Serial.println(config.mqttPort);
    mqttClient.connect();
}

void MqttManager::onMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
    ledControl.showSuccess();

    mqttClient.subscribe(getCommandTopic().c_str(), 2);
    publishDiscoveryMessage();
}

void MqttManager::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");
    ledControl.showError();
    if (WiFi.isConnected())
    {
        mqttReconnectTimer.once(2, [this]()
                                { this->connectToMqtt(); });
    }
}

void MqttManager::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                                size_t len, size_t index, size_t total)
{
    String message;
    for (size_t i = 0; i < len; i++)
        message += payload[i];

    Serial.print("Received MQTT message on topic: ");
    Serial.println(topic);
    Serial.println(message);
}

void MqttManager::publishDiscoveryMessage()
{
    String discoveryTopic = getDiscoveryTopic();
    JsonDocument doc;

    doc["name"] = getProductId();
    doc["unique_id"] = getProductId();
    doc["command_topic"] = getCommandTopic();
    doc["schema"] = "json";
    doc["brightness"] = true;
    doc["effect"] = true;

    JsonArray colorModes = doc["supported_color_modes"].to<JsonArray>();
    colorModes.add("rgb");

    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(discoveryTopic.c_str(), 0, true, payload.c_str());
    Serial.println("Discovery message published.");
}

String MqttManager::getProductId()
{
    return String(ESP.getChipId(), HEX);
}

String MqttManager::getDiscoveryTopic()
{
    char topic[128];
    snprintf(topic, sizeof(topic), DISCOVERY_TOPIC_TEMPLATE, getProductId().c_str());
    return String(topic);
}

String MqttManager::getCommandTopic()
{
    char topic[128];
    snprintf(topic, sizeof(topic), COMMAND_TOPIC_TEMPLATE, getProductId().c_str());
    return String(topic);
}