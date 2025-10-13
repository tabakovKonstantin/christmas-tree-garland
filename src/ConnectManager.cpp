#include "ConnectManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <ESP8266WiFi.h>

extern LedControl ledControl;
Config config;
AsyncWebServer server(80);

const char *PARAM_SSID = "ssid";
const char *PARAM_PASSWORD = "password";
const char *PARAM_MQTT_URL = "mqtt_url";
const char *PARAM_MQTT_PORT = "mqtt_port";
const char *PARAM_MQTT_USER = "mqtt_user";
const char *PARAM_MQTT_PASS = "mqtt_pass";

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
    ledControl.showError();
}

static String htmlEscape(const String &input)
{
    String s = input;
    s.replace("&", "&amp;");
    s.replace("<", "&lt;");
    s.replace(">", "&gt;");
    s.replace("\"", "&quot;");
    s.replace("'", "&#39;");
    return s;
}

void initWiFi()
{
    Serial.println();
    Serial.println("Initializing WiFi...");
    ledControl.showSuccess();

    WiFi.mode(WIFI_STA);
    bool tr = ConfigManager::loadConfig(config);
    if (tr)
    {
        Serial.println("Loaded config:");
        Serial.println(config.wifiSSID);
        Serial.println(config.wifiPassword);
        ledControl.showSuccess();

        WiFi.begin(config.wifiSSID, config.wifiPassword);
        unsigned long startAttemptTime = millis();
        const unsigned long timeout = 10000;

        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nConnected to Wi-Fi!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            ledControl.showSuccess();
            return;
        }

        Serial.println("\nFailed to connect to Wi-Fi.");
        ledControl.showError();
    }
    else
    {
        Serial.println("Config not loaded or missing, starting AP mode.");
        ledControl.showError();
    }

    Serial.println("Scanning Wi-Fi networks...");
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    Serial.print("Scan complete. Networks found: ");
    Serial.println(n);
    if (n > 0)
        ledControl.showSuccess();
    else
        ledControl.showError();

    for (int i = 0; i < n; ++i)
    {
        String ssid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);
        uint8_t encryption = WiFi.encryptionType(i);
        Serial.printf("%d: %s (RSSI %d) ENC:%d\n", i, ssid.c_str(), rssi, encryption);
    }

    WiFi.mode(WIFI_AP);
    String apSsid = getSsidWithChipId();
    WiFi.softAP(apSsid.c_str());
    Serial.print("AP SSID: ");
    Serial.println(apSsid);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
    ledControl.showSuccess();

    server.on("/", HTTP_GET, [n](AsyncWebServerRequest *request)
              {
        String html = "<h1>Wi-Fi & MQTT Configuration</h1>";
        html += "<form action='/config' method='POST'>";
        html += "<h3>Wi-Fi</h3>";

        html += "SSID: <select name='ssid'>";
        if (n <= 0) {
            html += "<option value='' disabled selected>-- no networks found --</option>";
        } else {
            html += "<option value='' selected>-- select SSID --</option>";
            for (int i = 0; i < n; ++i) {
                String ssidRaw = WiFi.SSID(i);
                String ssidEsc = htmlEscape(ssidRaw);
                html += "<option value='" + ssidEsc + "'>" + ssidEsc + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
            }
        }
        html += "</select><br>";

        html += "<br><label>Or enter hidden/custom SSID manually:</label><br>";
        html += "Manual SSID: <input type='text' name='ssid_manual'><br>";

        html += "Password: <input type='password' name='password'><br>";

        html += "<h3>MQTT</h3>";
        html += "Server URL: <input type='text' name='mqtt_url' value='116.203.170.149'><br>";
        html += "Port: <input type='number' name='mqtt_port' value='1883'><br>";
        html += "Username: <input type='text' name='mqtt_user' value='xmaslights'><br>";
        html += "Password: <input type='password' name='mqtt_pass'><br>";
        html += "<br><input type='submit' value='Save'>";
        html += "</form>";

        request->send(200, "text/html", html);
        ledControl.showSuccess(); });

    server.on("/config", HTTP_POST, handleConfigRequest);
    server.onNotFound(notFound);
    server.begin();
    ledControl.showSuccess();
}

void handleConfigRequest(AsyncWebServerRequest *request)
{
    String ssid = "";
    String manualSSID = "";
    String password = "";
    String mqttServer = "";
    int mqttPort = 1883;
    String mqttUser = "";
    String mqttPass = "";

    if (request->hasParam("ssid", true))
        ssid = request->getParam("ssid", true)->value();
    if (request->hasParam("ssid_manual", true))
        manualSSID = request->getParam("ssid_manual", true)->value();
    if (manualSSID.length() > 0)
        ssid = manualSSID;
    if (request->hasParam("password", true))
        password = request->getParam("password", true)->value();
    if (request->hasParam("mqtt_url", true))
        mqttServer = request->getParam("mqtt_url", true)->value();
    if (request->hasParam("mqtt_port", true))
        mqttPort = request->getParam("mqtt_port", true)->value().toInt();
    if (request->hasParam("mqtt_user", true))
        mqttUser = request->getParam("mqtt_user", true)->value();
    if (request->hasParam("mqtt_pass", true))
        mqttPass = request->getParam("mqtt_pass", true)->value();

    if (ssid.length() > 0 && password.length() > 0)
    {
        config.mqttServer = mqttServer;
        config.mqttPort = mqttPort;
        config.mqttUsername = mqttUser;
        config.mqttPassword = mqttPass;
        config.wifiSSID = ssid;
        config.wifiPassword = password;

        if (ConfigManager::saveConfig(config))
        {
            Serial.println("Configuration saved successfully.");
            ledControl.showSuccess();
        }
        else
        {
            Serial.println("Failed to save configuration.");
            ledControl.showError();
        }

        request->send(200, "text/html", "<h1>Configuration Saved! Rebooting...</h1>");
        delay(2000);
        ESP.restart();
    }
    else
    {
        Serial.println("Missing SSID or Password in config submission.");
        ledControl.showError();
        request->send(400, "text/html", "<h1>Missing SSID or Password</h1>");
    }
}

String getSsidWithChipId()
{
    uint32_t chipId = ESP.getChipId();
    char chipIdStr[11];
    itoa(chipId, chipIdStr, 10);
    return "Garland-" + String(chipIdStr);
}