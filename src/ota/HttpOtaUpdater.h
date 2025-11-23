#ifndef HTTP_OTA_UPDATER_H
#define HTTP_OTA_UPDATER_H

#include <Arduino.h>

class HttpOtaUpdater
{
public:
    static void updateFromUrl(const String &url);
};

#endif