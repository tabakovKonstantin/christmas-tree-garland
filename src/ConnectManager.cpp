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
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>🎄 Wi-Fi & MQTT Setup</title>
<style>
body {
  margin: 0;
  font-family: "Nunito", sans-serif;
  background: linear-gradient(180deg, #c91c24, #f4d35e);
  background-attachment: fixed;
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 20px;
}
.card {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 16px;
  box-shadow: 0 6px 20px rgba(0,0,0,0.2);
  padding: 24px;
  max-width: 420px;
  width: 100%;
  text-align: center;
  position: relative;
}
h1 {
  font-size: 1.6rem;
  color: #b91c1c;
  margin-bottom: 16px;
}
.section {
  margin: 18px 0;
  text-align: left;
}
label {
  display: block;
  font-weight: 600;
  margin-bottom: 6px;
  color: #444;
}
input, select {
  width: 100%;
  padding: 10px;
  border-radius: 8px;
  border: 1px solid #ccc;
  font-size: 1rem;
  box-sizing: border-box;
  margin-bottom: 12px;
}
button {
  background: #2ecc71;
  color: white;
  border: none;
  border-radius: 8px;
  font-size: 1.1rem;
  padding: 10px 16px;
  cursor: pointer;
  transition: background 0.3s, transform 0.2s;
}
button:hover {
  background: #27ae60;
  transform: scale(1.05);
}
.footer {
  font-size: 0.9rem;
  color: #666;
  margin-top: 10px;
}
</style>
</head>
<body>
<div class='card'>
<h1>🎄 Wi-Fi & MQTT Setup</h1>
<form action='/config' method='POST'>
<div class='section'>
<h3>📶 Wi-Fi</h3>
<label for='ssid'>Select SSID:</label>
<select id='ssid' name='ssid'>)rawliteral";

        if (n <= 0)
        {
            html += "<option value='' disabled selected>-- no networks found --</option>";
        }
        else
        {
            html += "<option value='' selected>-- select SSID --</option>";
            for (int i = 0; i < n; ++i)
            {
                String ssidRaw = WiFi.SSID(i);
                String ssidEsc = htmlEscape(ssidRaw);
                html += "<option value='" + ssidEsc + "'>" + ssidEsc + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
            }
        }

        html += R"rawliteral(
</select>
<label for='ssid_manual'>Or enter custom SSID:</label>
<input id='ssid_manual' type='text' name='ssid_manual' placeholder='Manual SSID'>
<label for='password'>Password:</label>
<input id='password' type='password' name='password' placeholder='Wi-Fi Password'>
</div>

<div class='section'>
<h3>🔌 MQTT</h3>
<label for='mqtt_url'>Server URL:</label>
<input id='mqtt_url' type='text' name='mqtt_url' placeholder='e.g. 116.203.170.149' value='116.203.170.149'>

<div style='display:flex; gap:10px;'>
  <div style='flex:2;'>
    <label for='mqtt_user'>Username:</label>
    <input id='mqtt_user' type='text' name='mqtt_user' placeholder='xmaslights' value='xmaslights'>
  </div>
  <div style='flex:1;'>
    <label for='mqtt_port'>Port:</label>
    <input id='mqtt_port' type='number' name='mqtt_port' value='1883'>
  </div>
</div>

<label for='mqtt_pass'>Password:</label>
<input id='mqtt_pass' type='password' name='mqtt_pass' placeholder='MQTT Password'>
</div>

<div class='section'>
<h3>💡 LED strip</h3>
<label for='color_order'>LED color order:</label>
<select id='color_order' name='color_order'>
  <option value='RGB' selected>RGB (default)</option>
  <option value='GRB'>GRB</option>
  <option value='RBG'>RBG</option>
  <option value='GBR'>GBR</option>
  <option value='BRG'>BRG</option>
  <option value='BGR'>BGR</option>
</select>
</div>

<button type='submit'>🎅 Save Configuration</button>
<p class='footer'>Made with ❤️ for Christmas</p>
</form>
</div>
</body>
</html>)rawliteral";

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
    String colorOrder = "RGB"; // default RGB

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
    if (request->hasParam("color_order", true))
        colorOrder = request->getParam("color_order", true)->value();

    if (ssid.length() > 0 && password.length() > 0)
    {
        config.mqttServer = mqttServer;
        config.mqttPort = mqttPort;
        config.mqttUsername = mqttUser;
        config.mqttPassword = mqttPass;
        config.wifiSSID = ssid;
        config.wifiPassword = password;
        config.colorOrder = colorOrder;

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