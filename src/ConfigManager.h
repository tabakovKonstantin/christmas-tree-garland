#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "FileManager.h"
#include "Config.h"

#define CONFIG_FILE "/config.txt"

class ConfigManager {
public:
    static bool saveConfig(const Config& config) {
        String jsonString = config.toJson();
        return FileManager::saveToFile(CONFIG_FILE, jsonString);
    }

    static bool loadConfig(Config& config) {
        String jsonString = FileManager::loadFromFile(CONFIG_FILE);
        if (jsonString.isEmpty()) {
            return false;
        }
        return config.fromJson(jsonString);
    }

    static void eraseConfig() {
        FileManager::removeFile(CONFIG_FILE);
    }
};

#endif
