#ifndef HTTP_OTA_UPDATER_H
#define HTTP_OTA_UPDATER_H

#include <Arduino.h>

// HTTP-based OTA updater for ESP8266.
// Триггерится из MQTT: в payload прилетает URL прошивки.
// Реализация теперь на асинхронном TCP-клиенте (ESPAsyncTCP / AsyncClient):
//  - создаём AsyncClient
//  - шлём HTTP GET в onConnect
//  - в onData парсим статус/заголовки и льём тело в Update.
//  - по завершении прошивки перезагружаем устройство.
class HttpOtaUpdater
{
public:
    // Запустить OTA по указанному URL.
    // Пример: "http://192.168.20.5:8080/firmware.bin"
    //
    // Ограничения:
    //  - Только HTTP (http://). HTTPS не поддерживается.
    //  - Сервер должен отдавать прошивку с кодом 200 и заголовком Content-Length.
    static void updateFromUrl(const String &url);
};

#endif