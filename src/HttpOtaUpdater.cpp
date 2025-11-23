#include "HttpOtaUpdater.h"

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Updater.h>

// ================== ВНУТРЕННЕЕ СОСТОЯНИЕ OTA (АСИНХРОННОЕ) ==================

namespace
{
    struct ParsedUrl
    {
        String host;
        String path;
        uint16_t port;
        bool https;
        bool valid;

        ParsedUrl() : port(80), https(false), valid(false) {}
    };

    // Текущее состояние OTA-сессии
    AsyncClient *otaClient = nullptr;
    bool otaInProgress = false;

    ParsedUrl currentUrl;

    // Парсинг HTTP-ответа
    String headerBuffer;          // копим заголовки до \r\n\r\n
    bool headersComplete = false; // заголовки прочитаны и распарсены

    long contentLength = -1;
    bool isChunked = false;

    size_t receivedBody = 0;
    bool updateBegun = false;
    bool updateEnded = false;

    // ---------- Вспомогательные функции ----------

    // Сбрасывает временное состояние одной OTA-сессии, НО НЕ трогает currentUrl и otaInProgress.
    static void initOtaSessionState()
    {
        headerBuffer = "";
        headersComplete = false;
        contentLength = -1;
        isChunked = false;
        receivedBody = 0;
        updateBegun = false;
        updateEnded = false;
    }

    // Полный сброс OTA: и временное состояние, и URL, и флаг otaInProgress.
    static void resetOtaState()
    {
        initOtaSessionState();
        currentUrl = ParsedUrl();
        otaInProgress = false;
    }

    static void freeOtaClient()
    {
        if (otaClient != nullptr)
        {
            otaClient->onConnect(nullptr, nullptr);
            otaClient->onDisconnect(nullptr, nullptr);
            otaClient->onError(nullptr, nullptr);
            otaClient->onTimeout(nullptr, nullptr);
            otaClient->onData(nullptr, nullptr);
            otaClient->close(true);
            otaClient->free();
            delete otaClient;
            otaClient = nullptr;
        }
    }

    // Примитивный парсер URL, как раньше, но только для http/https.
    static ParsedUrl parseUrl(const String &url)
    {
        ParsedUrl result;

        if (!url.startsWith("http://") && !url.startsWith("https://"))
        {
            Serial.println("[HTTP-OTA] URL must start with http:// or https://");
            return result;
        }

        bool isHttps = url.startsWith("https://");
        int schemeLen = isHttps ? 8 : 7;

        int hostStart = schemeLen;
        if (hostStart >= (int)url.length())
        {
            Serial.println("[HTTP-OTA] URL parse error: no host part");
            return result;
        }

        int pathStart = url.indexOf('/', hostStart);

        String hostPort;
        if (pathStart < 0)
        {
            hostPort = url.substring(hostStart);
            result.path = "/";
        }
        else
        {
            hostPort = url.substring(hostStart, pathStart);
            result.path = url.substring(pathStart);
            if (result.path.length() == 0)
            {
                result.path = "/";
            }
        }

        int colonPos = hostPort.indexOf(':');
        if (colonPos >= 0)
        {
            result.host = hostPort.substring(0, colonPos);
            String portStr = hostPort.substring(colonPos + 1);
            int parsedPort = portStr.toInt();
            if (parsedPort <= 0 || parsedPort > 65535)
            {
                Serial.print("[HTTP-OTA] Invalid port in URL: ");
                Serial.println(portStr);
                return result;
            }
            result.port = static_cast<uint16_t>(parsedPort);
        }
        else
        {
            result.host = hostPort;
            result.port = isHttps ? 443 : 80;
        }

        if (result.host.length() == 0)
        {
            Serial.println("[HTTP-OTA] URL parse error: empty host");
            return result;
        }

        result.https = isHttps;
        result.valid = true;

        Serial.print("[HTTP-OTA] Parsed URL -> host: ");
        Serial.print(result.host);
        Serial.print(" port: ");
        Serial.print(result.port);
        Serial.print(" path: ");
        Serial.println(result.path);

        return result;
    }

    // Разбор заголовков из headerBuffer (всё до \r\n\r\n)
    static bool parseHttpHeaders()
    {
        // Ищем первую строку статуса
        int firstCRLF = headerBuffer.indexOf("\r\n");
        if (firstCRLF <= 0)
        {
            Serial.println("[HTTP-OTA] Invalid HTTP response: no status line.");
            return false;
        }

        String statusLine = headerBuffer.substring(0, firstCRLF);
        statusLine.trim();
        Serial.print("[HTTP-OTA] HTTP status line: ");
        Serial.println(statusLine);

        int firstSpace = statusLine.indexOf(' ');
        int secondSpace = statusLine.indexOf(' ', firstSpace + 1);
        int statusCode = -1;
        if (firstSpace > 0 && secondSpace > firstSpace)
        {
            statusCode = statusLine.substring(firstSpace + 1, secondSpace).toInt();
        }

        Serial.print("[HTTP-OTA] Parsed HTTP status code: ");
        Serial.println(statusCode);

        if (statusCode != 200)
        {
            Serial.println("[HTTP-OTA] HTTP code is not 200, aborting OTA.");
            Serial.println("           Проверь, что по URL отдаётся бинарник без редиректов.");
            return false;
        }

        // Разбираем остальные заголовки
        String headersPart = headerBuffer.substring(firstCRLF + 2); // после первой \r\n
        contentLength = -1;
        isChunked = false;

        int pos = 0;
        while (true)
        {
            int next = headersPart.indexOf("\r\n", pos);
            if (next < 0)
            {
                break;
            }
            if (next == pos)
            {
                // Пустая строка (теоретически конец заголовков), но мы уже отрезали по \r\n\r\n,
                // так что сюда обычно не попадём.
                break;
            }

            String line = headersPart.substring(pos, next);
            line.trim();

            String lower = line;
            lower.toLowerCase();

            Serial.print("[HTTP-OTA] Header: ");
            Serial.println(line);

            if (lower.startsWith("content-length:"))
            {
                int colon = line.indexOf(':');
                if (colon >= 0)
                {
                    String v = line.substring(colon + 1);
                    v.trim();
                    contentLength = v.toInt();
                }
            }
            else if (lower.startsWith("transfer-encoding:") &&
                     lower.indexOf("chunked") >= 0)
            {
                isChunked = true;
            }

            pos = next + 2;
        }

        if (isChunked)
        {
            Serial.println("[HTTP-OTA] Chunked transfer encoding is not supported for OTA.");
            Serial.println("           Нужен обычный Content-Length без chunked.");
            return false;
        }

        if (contentLength <= 0)
        {
            Serial.println("[HTTP-OTA] Missing or invalid Content-Length, cannot proceed.");
            return false;
        }

        Serial.print("[HTTP-OTA] Firmware size (Content-Length): ");
        Serial.println(contentLength);

        // Проверяем размер под скетч, как обычно.
        size_t freeSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        Serial.print("[HTTP-OTA] Free sketch space: ");
        Serial.println(freeSketchSpace);

        if ((size_t)contentLength > freeSketchSpace)
        {
            Serial.println("[HTTP-OTA] Firmware is too large for available flash space.");
            return false;
        }

        // Инициализируем Update
        if (!Update.begin(contentLength))
        {
            Serial.println("[HTTP-OTA] Update.begin() failed:");
            Update.printError(Serial);
            return false;
        }

        updateBegun = true;
        Serial.println("[HTTP-OTA] Flash update started (async).");
        return true;
    }

    // ---------- Callbacks AsyncClient ----------

    static void onOtaConnect(void *arg, AsyncClient *client)
    {
        Serial.println("[HTTP-OTA] AsyncClient connected, sending HTTP GET...");

        String hostHeader = currentUrl.host;
        String request = String("GET ") + currentUrl.path + " HTTP/1.1\r\n" +
                         "Host: " + hostHeader + "\r\n" +
                         "User-Agent: ESP8266-ASYNC-OTA/1.0\r\n" +
                         "Connection: close\r\n\r\n";

        client->write(request.c_str(), request.length());
    }

    static void onOtaError(void *arg, AsyncClient *client, err_t error)
    {
        Serial.print("[HTTP-OTA] AsyncClient error: ");
        Serial.println((int)error);

        if (updateBegun && !updateEnded)
        {
            Serial.println("[HTTP-OTA] Update aborted due to TCP error.");
            Update.printError(Serial);
        }

        resetOtaState();
        freeOtaClient();
    }

    static void onOtaTimeout(void *arg, AsyncClient *client, uint32_t time)
    {
        Serial.print("[HTTP-OTA] AsyncClient timeout (ms): ");
        Serial.println(time);

        if (updateBegun && !updateEnded)
        {
            Serial.println("[HTTP-OTA] Update aborted due to timeout.");
            Update.printError(Serial);
        }

        resetOtaState();
        freeOtaClient();
    }

    static void onOtaDisconnect(void *arg, AsyncClient *client)
    {
        Serial.println("[HTTP-OTA] AsyncClient disconnected.");

        // Если мы уже завершили Update.end() и всё в порядке — перезагружаемся.
        if (updateBegun && updateEnded && Update.isFinished())
        {
            Serial.println("[HTTP-OTA] OTA update successful, rebooting...");
            resetOtaState();
            freeOtaClient();
            Serial.println("===========================================");
            ESP.restart();
            return; // на всякий случай
        }

        // Иначе что-то пошло не так
        if (updateBegun && !updateEnded)
        {
            Serial.println("[HTTP-OTA] Disconnected before Update.end().");
            Update.printError(Serial);
        }

        resetOtaState();
        freeOtaClient();
        Serial.println("===========================================");
    }

    static void onOtaData(void *arg, AsyncClient *client, void *data, size_t len)
    {
        uint8_t *bytes = static_cast<uint8_t *>(data);

        // Ограничим размер headerBuffer — если вдруг сервак шлёт мусор
        if (!headersComplete && headerBuffer.length() > 2048)
        {
            Serial.println("[HTTP-OTA] Headers too large, aborting.");
            client->close(true);
            return;
        }

        size_t index = 0;

        // 1) Сначала добираем заголовки, пока не встретим \r\n\r\n
        if (!headersComplete)
        {
            while (index < len && !headersComplete)
            {
                char c = static_cast<char>(bytes[index++]);
                headerBuffer += c;

                int hbLen = headerBuffer.length();
                if (hbLen >= 4 &&
                    headerBuffer[hbLen - 4] == '\r' &&
                    headerBuffer[hbLen - 3] == '\n' &&
                    headerBuffer[hbLen - 2] == '\r' &&
                    headerBuffer[hbLen - 1] == '\n')
                {
                    headersComplete = true;
                    break;
                }
            }

            if (headersComplete)
            {
                // headerBuffer сейчас содержит всё до \r\n\r\n (включая его).
                // Парсим.
                if (!parseHttpHeaders())
                {
                    Serial.println("[HTTP-OTA] Header parse failed, closing connection.");
                    client->close(true);
                    return;
                }

                // Если в этом же чанке есть кусок тела — пишем его в Update.
                if (index < len && updateBegun)
                {
                    size_t bodyLen = len - index;
                    size_t written = Update.write(bytes + index, bodyLen);
                    receivedBody += written;

                    if (written != bodyLen)
                    {
                        Serial.println("[HTTP-OTA] Mismatch between received and written bytes in first body chunk.");
                        Update.printError(Serial);
                        client->close(true);
                        return;
                    }

                    Serial.print("[HTTP-OTA] Written bytes: ");
                    Serial.print(receivedBody);
                    Serial.print(" / ");
                    Serial.println(contentLength);
                }
                return;
            }
            else
            {
                // Пока ещё только заголовки, тела нет.
                return;
            }
        }

        // 2) Если заголовки уже прочитаны — весь буфер это тело
        if (updateBegun && !updateEnded)
        {
            size_t written = Update.write(bytes, len);
            receivedBody += written;

            if (written != len)
            {
                Serial.println("[HTTP-OTA] Mismatch between received and written bytes in body.");
                Update.printError(Serial);
                client->close(true);
                return;
            }

            Serial.print("[HTTP-OTA] Written bytes: ");
            Serial.print(receivedBody);
            Serial.print(" / ");
            Serial.println(contentLength);

            // Если всё тело получено — завершаем Update.
            if (receivedBody >= (size_t)contentLength)
            {
                Serial.println("[HTTP-OTA] All firmware bytes received, calling Update.end()...");

                if (!Update.end())
                {
                    Serial.println("[HTTP-OTA] Update.end() failed:");
                    Update.printError(Serial);
                    client->close(true);
                    return;
                }

                updateEnded = true;

                if (!Update.isFinished())
                {
                    Serial.println("[HTTP-OTA] Update not finished, something went wrong.");
                    Update.printError(Serial);
                    client->close(true);
                    return;
                }

                Serial.println("[HTTP-OTA] Update finished successfully, will reboot on disconnect.");
                // Закрываем соединение, дальше сработает onOtaDisconnect и сделает ESP.restart().
                client->close(true);
            }
        }
    }

} // namespace

// ================== ПУБЛИЧНЫЙ ИНТЕРФЕЙС ==================

void HttpOtaUpdater::updateFromUrl(const String &url)
{
    Serial.println();
    Serial.println("========== MQTT HTTP OTA REQUEST ==========");

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[HTTP-OTA] Wi-Fi is not connected, cannot start update.");
        Serial.println("===========================================");
        return;
    }

    if (otaInProgress)
    {
        Serial.println("[HTTP-OTA] Another OTA is already in progress, skipping.");
        Serial.println("===========================================");
        return;
    }

    Serial.print("[HTTP-OTA] Requested URL: ");
    Serial.println(url);

    Serial.print("[HTTP-OTA] Local IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("[HTTP-OTA] Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("[HTTP-OTA] Subnet: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("[HTTP-OTA] RSSI: ");
    Serial.println(WiFi.RSSI());

    // Парсим URL и сохраняем в currentUrl
    currentUrl = parseUrl(url);
    if (!currentUrl.valid)
    {
        Serial.println("[HTTP-OTA] Parsed URL is invalid, aborting OTA.");
        Serial.println("===========================================");
        return;
    }

    if (currentUrl.https)
    {
        Serial.println("[HTTP-OTA] HTTPS is not supported for OTA.");
        Serial.println("           Нужен URL вида http://<ip>/firmware.bin (без https).");
        Serial.println("===========================================");
        return;
    }

    // Создаём асинхронный клиент (тот же стек, что и у AsyncMqttClient/ESPAsyncTCP).
    otaClient = new AsyncClient();
    if (!otaClient)
    {
        Serial.println("[HTTP-OTA] Failed to allocate AsyncClient.");
        Serial.println("===========================================");
        return;
    }

    // Инициализируем сессионное состояние, но НЕ трогаем currentUrl.
    initOtaSessionState();
    otaInProgress = true;

    otaClient->onConnect(onOtaConnect, nullptr);
    otaClient->onData(onOtaData, nullptr);
    otaClient->onError(onOtaError, nullptr);
    otaClient->onTimeout(onOtaTimeout, nullptr);
    otaClient->onDisconnect(onOtaDisconnect, nullptr);

    otaClient->setRxTimeout(8);        // таймаут приёма данных (секунды)
    otaClient->setAckTimeout(8);       // таймаут ACK
    otaClient->setNoDelay(true);       // отключаем Nagle

    Serial.print("[HTTP-OTA] Async connecting to ");
    Serial.print(currentUrl.host);
    Serial.print(":");
    Serial.println(currentUrl.port);

    IPAddress ip;
    bool useIp = ip.fromString(currentUrl.host);

    bool started = false;
    if (useIp)
    {
        started = otaClient->connect(ip, currentUrl.port);
    }
    else
    {
        started = otaClient->connect(currentUrl.host.c_str(), currentUrl.port);
    }

    if (!started)
    {
        Serial.println("[HTTP-OTA] AsyncClient.connect() could not start.");
        freeOtaClient();
        resetOtaState();
        Serial.println("===========================================");
        return;
    }

    Serial.println("[HTTP-OTA] AsyncClient.connect() initiated.");
    Serial.println("===========================================");
}