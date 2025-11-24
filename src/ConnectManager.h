#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h> 
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>

void initWiFi();
String getSsidWithChipId();
void handleConfigRequest(AsyncWebServerRequest *request);

#endif
