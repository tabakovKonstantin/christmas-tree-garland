#ifndef PAYLOAD_H
#define PAYLOAD_H

#include <ArduinoJson.h>

class Payload
{
public:
    int brightness;
    String color_mode; // логически константное значение "rgb"
    struct Color
    {
        int r, g, b, c, w;
    } color;
    String effect;
    String state;
    int transition;

    Payload()
        : brightness(-1),
          color_mode("rgb"),
          effect("null"),
          state("OFF"),
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
        doc["brightness"] = brightness;

        // color_mode всегда константа "rgb", не зависит от входящих команд
        doc["color_mode"] = "rgb";

        doc["color"]["r"] = color.r;
        doc["color"]["g"] = color.g;
        doc["color"]["b"] = color.b;

        doc["effect"] = effect;
        doc["state"] = state;
        doc["transition"] = transition;

        String output;
        serializeJson(doc, output);
        return output;
    }

    bool fromJson(const String &json)
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.println("Ошибка десериализации JSON");
            return false;
        }

        // Если поле не пришло — ставим -1 как "нет значения".
        brightness = doc["brightness"].isNull() ? -1 : doc["brightness"];

        // color_mode и color_temp из входящего JSON полностью игнорируем,
        // мы всегда работаем в режиме "rgb".
        color_mode = "rgb";

        color.r = doc["color"]["r"].isNull() ? -1 : doc["color"]["r"];
        color.g = doc["color"]["g"].isNull() ? -1 : doc["color"]["g"];
        color.b = doc["color"]["b"].isNull() ? -1 : doc["color"]["b"];
        color.c = -1;
        color.w = -1;

        effect = doc["effect"].as<String>();
        state = doc["state"].as<String>();
        transition = doc["transition"];

        return true;
    }
};

#endif