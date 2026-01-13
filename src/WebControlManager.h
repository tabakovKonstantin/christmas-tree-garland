#ifndef WEB_CONTROL_MANAGER_H
#define WEB_CONTROL_MANAGER_H

#include <ESPAsyncWebServer.h>
#include "LedControl.h"

class WebControlManager {
public:
    WebControlManager(LedControl& ledControl);
    void setup(AsyncWebServer& server);

private:
    LedControl& ledControl;
    void handleApiSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
};

#endif