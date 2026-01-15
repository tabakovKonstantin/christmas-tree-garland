#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#include <ArduinoJson.h>

class Config {
public:
    // 0: Standalone (Web)
    // 1: MQTT Only
    // 2: Both
    int workMode = 0; 
    
    String mqttServer;
    int mqttPort = 1883; 
    String mqttUsername;
    String mqttPassword;
    String wifiSSID;
    String wifiPassword;
    String colorOrder; 

    String toJson() const {
        JsonDocument jsonDoc; 
        jsonDoc["workMode"] = workMode;
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
            Serial.println("JSON parse error: " + String(error.c_str()));
            return false;
        }

        // Backward compatibility: check if "mqttEnabled" exists if "workMode" doesn't
        if (jsonDoc.containsKey("workMode")) {
            workMode = jsonDoc["workMode"].as<int>();
        } else if (jsonDoc.containsKey("mqttEnabled")) {
            // Migration: if enabled=true -> Both(2), else Standalone(0)
            bool en = jsonDoc["mqttEnabled"];
            workMode = en ? 2 : 0;
        } else {
            workMode = 0;
        }

        mqttServer = jsonDoc["mqttServer"].as<String>();
        mqttPort = jsonDoc["mqttPort"].as<int>();
        if (mqttPort <= 0) mqttPort = 1883; 

        mqttUsername = jsonDoc["mqttUsername"].as<String>();
        mqttPassword = jsonDoc["mqttPassword"].as<String>();
        wifiSSID = jsonDoc["wifiSSID"].as<String>();
        wifiPassword = jsonDoc["wifiPassword"].as<String>();
        colorOrder = jsonDoc["colorOrder"].as<String>();
        return true;
    }
};

#endif