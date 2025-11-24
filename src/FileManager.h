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
            Serial.println("Ошибка инициализации файловой системы");
            ledControl.showError();
        }
        else
        {
            Serial.println("Файловая система инициализирована успешно");
            ledControl.showSuccess();
        }
        return result;
    }

    static bool saveToFile(const char *filename, const String &data)
    {
        File file = LittleFS.open(filename, "w");
        if (!file)
        {
            Serial.println();
            Serial.print("Ошибка открытия файла для записи: ");
            Serial.println(filename);
            ledControl.showError();
            return false;
        }
        file.print(data);
        file.close();
        Serial.println();
        Serial.print("Записано в файл: ");
        Serial.print(filename);
        Serial.print(" данные: ");
        Serial.println(data);
        ledControl.showSuccess();
        return true;
    }

    static String loadFromFile(const char *filename)
    {
        File file = LittleFS.open(filename, "r");
        if (!file)
        {
            Serial.println();
            Serial.print("Ошибка открытия файла для чтения: ");
            Serial.println(filename);
            ledControl.showError();
            return "";
        }
        String data = file.readString();
        file.close();
        Serial.println();
        Serial.print("Прочтено из файла: ");
        Serial.print(filename);
        Serial.print(" данные: ");
        Serial.println(data);
        ledControl.showSuccess();
        return data;
    }

    static void removeFile(const char *filename)
    {
        if (LittleFS.exists(filename))
        {
            if (LittleFS.remove(filename))
            {
                Serial.println("File removed successfully.");
                ledControl.showSuccess();
            }
            else
            {
                Serial.println("Failed to remove file.");
                ledControl.showError();
            }
            return;
        }
        Serial.println("File does not exist.");
        ledControl.showError();
        return;
    }
};

#endif