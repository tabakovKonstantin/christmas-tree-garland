#ifndef WEB_CONTROL_MANAGER_H
#define WEB_CONTROL_MANAGER_H

#include <ESPAsyncWebServer.h>
#include "LedControl.h"

class WebControlManager {
public:
    WebControlManager(LedControl& ledControl);
    void setup(AsyncWebServer& server, bool fullUI = true);

private:
    LedControl& ledControl;
};

#endif