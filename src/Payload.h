#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <ArduinoJson.h>

class Payload
{
public:
    int brightness;
    String color_mode; 
    struct Color { int r, g, b, c, w; } color;
    String effect; // "" = no change, "null" = stop, "Name" = start
    String state;  // "" = no change, "ON", "OFF"
    int transition;

    Payload()
        : brightness(-1),
          color_mode("rgb"),
          effect(""), // Default to empty (no action)
          state(""),  // Default to empty (no action)
          transition(0)
    {
        color.r = -1;
        color.g = -1;
        color.b = -1;
        color.c = -1;
        color.w = -1;
    }

    String toJson() const
    {
        JsonDocument doc;
        if (brightness != -1) doc["brightness"] = brightness;
        doc["color_mode"] = "rgb";
        if (color.r != -1) {
            doc["color"]["r"] = color.r;
            doc["color"]["g"] = color.g;
            doc["color"]["b"] = color.b;
        }
        if (effect.length() > 0) doc["effect"] = effect;
        if (state.length() > 0) doc["state"] = state;
        
        String output;
        serializeJson(doc, output);
        return output;
    }

    bool fromJson(const String &json)
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error) {
            Serial.println("JSON Error");
            return false;
        }

        if (!doc["brightness"].isNull()) brightness = doc["brightness"];
        
        if (!doc["color"]["r"].isNull()) {
            color.r = doc["color"]["r"];
            color.g = doc["color"]["g"];
            color.b = doc["color"]["b"];
        }

        if (!doc["effect"].isNull()) {
            effect = doc["effect"].as<String>();
        } else {
            effect = ""; // Explicitly reset to "no change" if missing
        }

        if (!doc["state"].isNull()) {
            state = doc["state"].as<String>();
        } else {
            state = "";
        }

        return true;
    }
};

#endif