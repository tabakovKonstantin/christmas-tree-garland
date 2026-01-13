#ifndef STATE_STORAGE_H
#define STATE_STORAGE_H

#include <LittleFS.h>
#include "Payload.h"

#define STATE_FILE "/state.json"

class StateStorage {
public:
    static bool saveState(const Payload& state) {
        File file = LittleFS.open(STATE_FILE, "w");
        if (!file) {
            return false;
        }
        String json = state.toJson();
        file.print(json);
        file.close();
        return true;
    }

    static bool loadState(Payload& state) {
        if (!LittleFS.exists(STATE_FILE)) return false;
        File file = LittleFS.open(STATE_FILE, "r");
        if (!file) return false;
        String json = file.readString();
        file.close();
        return state.fromJson(json);
    }
};

#endif