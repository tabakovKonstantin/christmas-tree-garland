#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#include <ArduinoJson.h>

class Config {
public:
    String mqttServer;
    int mqttPort;
    String mqttUsername;
    String mqttPassword;
    String wifiSSID;
    String wifiPassword;
    String colorOrder; // LED color order, e.g. "GRB"

    String toJson() const {
        JsonDocument jsonDoc; 
        jsonDoc["mqttServer"] = mqttServer;
        jsonDoc["mqttPort"] = mqttPort;
        jsonDoc["mqttUsername"] = mqttUsername;
        jsonDoc["mqttPassword"] = mqttPassword;
        jsonDoc["wifiSSID"] = wifiSSID;
        jsonDoc["wifiPassword"] = wifiPassword;
        jsonDoc["colorOrder"] = colorOrder;

        String jsonString;
        serializeJson(jsonDoc, jsonString);
        return jsonString;
    }

    bool fromJson(const String& jsonString) {
        JsonDocument jsonDoc;
        DeserializationError error = deserializeJson(jsonDoc, jsonString);
        if (error) {
            Serial.println("Ошибка парсинга JSON: " + String(error.c_str()));
            return false;
        }

        mqttServer = jsonDoc["mqttServer"].as<String>();
        mqttPort = jsonDoc["mqttPort"].as<int>();
        mqttUsername = jsonDoc["mqttUsername"].as<String>();
        mqttPassword = jsonDoc["mqttPassword"].as<String>();
        wifiSSID = jsonDoc["wifiSSID"].as<String>();
        wifiPassword = jsonDoc["wifiPassword"].as<String>();
        colorOrder = jsonDoc["colorOrder"].as<String>();
        return true;
    }

};

#endif // MY_CONFIG_H