#include "MqttManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <Ticker.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

extern LedControl ledControl;

MqttManager::MqttManager(LedControl &led_Control)
    : ledControl(led_Control),
      commandTopic(""),
      otaTopic(""),
      stateTopic(""),
      router(),
      lightHandler(led_Control),
      otaHandler(led_Control)
{
    // Build topics once based on chip ID.
    commandTopic = buildCommandTopic();
    otaTopic = buildOtaTopic();
    stateTopic = buildStateTopic();

    // Configure handlers with their dedicated topics.
    lightHandler.setTopic(commandTopic);
    otaHandler.setTopic(otaTopic);

    // Register handlers in router.
    router.addHandler(&lightHandler);
    router.addHandler(&otaHandler);
}

void MqttManager::init()
{
    Serial.println();
    Serial.println("Initializing MQTT Manager...");

    mqttClient.onConnect([this](bool sessionPresent)
                         { this->onMqttConnect(sessionPresent); });
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason)
                            { this->onMqttDisconnect(reason); });
    mqttClient.onMessage([this](char *topic, char *payload,
                                AsyncMqttClientMessageProperties properties,
                                size_t len, size_t index, size_t total)
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

    Serial.print("Subscribing to command topic: ");
    Serial.println(commandTopic);
    mqttClient.subscribe(commandTopic.c_str(), 2);

    Serial.print("Subscribing to OTA topic: ");
    Serial.println(otaTopic);
    mqttClient.subscribe(otaTopic.c_str(), 1);

    publishDiscoveryMessage();
    publishInitialState();
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

void MqttManager::onMqttMessage(char *topic,
                                char *payload,
                                AsyncMqttClientMessageProperties properties,
                                size_t len,
                                size_t index,
                                size_t total)
{
    Serial.println();
    Serial.println("Message received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  payload: ");

    String message;
    message.reserve(len + 1);
    for (size_t i = 0; i < len; i++)
    {
        message += payload[i];
    }
    Serial.println(message);

    String topicStr(topic);

    // Delegate logic to router and handlers.
    router.route(topicStr, message);

    // If this was a command to change light state, publish the new state.
    if (topicStr == commandTopic)
    {
        // Reuse the incoming payload as state representation.
        mqttClient.publish(stateTopic.c_str(), 1, true, message.c_str());
    }
}

void MqttManager::publishDiscoveryMessage()
{
    JsonDocument doc;
    doc["name"] = "Christmas garland";
    doc["unique_id"] = getProductId();
    doc["command_topic"] = commandTopic;
    doc["state_topic"] = stateTopic;

    JsonObject device = doc["device"].to<JsonObject>();
    JsonArray identifiers = device["identifiers"].to<JsonArray>();
    identifiers.add(getProductId());
    device["manufacturer"] = "Tabakov";
    device["model"] = "Home";
    device["name"] = "Christmas garland";
    device["sw_version"] = "0.0.1";

    JsonArray colorModes = doc["supported_color_modes"].to<JsonArray>();
    colorModes.add("rgb");

    doc["effect"] = true;
    JsonArray effectList = doc["effect_list"].to<JsonArray>();
    effectList.add("Rainbow");
    effectList.add("Smooth wave");
    effectList.add("Sparkle");
    effectList.add("Tree");
    effectList.add("Halloween Flame");

    doc["schema"] = "json";
    doc["optimistic"] = true;

    String message;
    serializeJson(doc, message);

    String discoveryTopic = buildDiscoveryTopic();
    mqttClient.publish(discoveryTopic.c_str(), 1, true, message.c_str());
}

void MqttManager::publishInitialState()
{
    // Build a minimal initial state payload for Home Assistant.
    Payload p;
    p.brightness = BRIGHTNESS;
    p.color_mode = "rgb";
    p.color_temp = 0;

    p.color.r = -1;
    p.color.g = -1;
    p.color.b = -1;
    p.color.c = -1;
    p.color.w = -1;

    p.effect = "null";
    p.state = "OFF";
    p.transition = 0;

    String json = p.toJson();
    mqttClient.publish(stateTopic.c_str(), 1, true, json.c_str());
}

String MqttManager::getProductId()
{
    char chipIdStr[11];
    itoa(ESP.getChipId(), chipIdStr, 10);
    return "garland-" + String(chipIdStr);
}

String MqttManager::buildDiscoveryTopic() const
{
    String productId = const_cast<MqttManager *>(this)->getProductId();
    char discoveryTopic[100];
    snprintf(discoveryTopic, sizeof(discoveryTopic), DISCOVERY_TOPIC_TEMPLATE, productId.c_str());
    return String(discoveryTopic);
}

String MqttManager::buildCommandTopic() const
{
    String productId = const_cast<MqttManager *>(this)->getProductId();
    char commandTopicBuf[100];
    snprintf(commandTopicBuf, sizeof(commandTopicBuf), COMMAND_TOPIC_TEMPLATE, productId.c_str());
    return String(commandTopicBuf);
}

String MqttManager::buildOtaTopic() const
{
    String productId = const_cast<MqttManager *>(this)->getProductId();
    char otaTopicBuf[100];
    snprintf(otaTopicBuf, sizeof(otaTopicBuf), OTA_TOPIC_TEMPLATE, productId.c_str());
    return String(otaTopicBuf);
}

String MqttManager::buildStateTopic() const
{
    String productId = const_cast<MqttManager *>(this)->getProductId();
    char stateTopicBuf[100];
    snprintf(stateTopicBuf, sizeof(stateTopicBuf), STATE_TOPIC_TEMPLATE, productId.c_str());
    return String(stateTopicBuf);
}