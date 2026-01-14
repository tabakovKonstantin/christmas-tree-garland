#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <LittleFS.h>
#include "LedControl.h"

extern LedControl ledControl;

class FileManager
{
public:
    static bool begin()
    {
        bool result = LittleFS.begin();
        if (!result)
        {
            Serial.println("[FS] Error: LittleFS mount failed!");
            ledControl.showError();
        }
        else
        {
            Serial.println("[FS] Mounted successfully.");
        }
        return result;
    }

    static bool saveToFile(const char *filename, const String &data)
    {
        File file = LittleFS.open(filename, "w");
        if (!file)
        {
            Serial.print("[FS] Error opening file for write: ");
            Serial.println(filename);
            ledControl.showError();
            return false;
        }
        file.print(data);
        file.close();
        
        Serial.print("[FS] Saved to ");
        Serial.print(filename);
        Serial.print(" (");
        Serial.print(data.length());
        Serial.println(" bytes)");
        
        ledControl.showSuccess();
        return true;
    }

    static String loadFromFile(const char *filename)
    {
        if (!LittleFS.exists(filename)) {
            // It's not an error, just first run or reset state
            Serial.print("[FS] File not found: ");
            Serial.println(filename);
            return "";
        }

        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            Serial.print("[FS] Error opening file for read: ");
            Serial.println(filename);
            ledControl.showError();
            return "";
        }
        
        String data = file.readString();
        file.close();
        
        Serial.print("[FS] Loaded from ");
        Serial.println(filename);
        return data;
    }

    static void removeFile(const char *filename)
    {
        if (LittleFS.exists(filename))
        {
            LittleFS.remove(filename);
            Serial.print("[FS] Removed: ");
            Serial.println(filename);
        }
    }
};

#endif