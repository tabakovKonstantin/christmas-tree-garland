#include "ConnectManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <ESP8266WiFi.h>

extern LedControl ledControl;
Config config;
AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void initWiFi()
{
    WiFi.mode(WIFI_STA);
    if (ConfigManager::loadConfig(config)) {
        WiFi.begin(config.wifiSSID, config.wifiPassword);
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(500); }
        if (WiFi.status() == WL_CONNECTED) return;
    }

    WiFi.mode(WIFI_AP);
    String apSsid = getSsidWithChipId();
    WiFi.softAP(apSsid.c_str());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name='viewport' content='width=device-width, initial-scale=1'>
    <style>
        body { font-family: sans-serif; background: #c91c24; color: white; text-align: center; }
        .container { background: white; color: #333; margin: 20px; padding: 20px; border-radius: 15px; }
        input, select { width: 100%; padding: 10px; margin: 10px 0; box-sizing: border-box; }
        button { background: #2ecc71; color: white; border: none; padding: 15px; width: 100%; border-radius: 10px; }
        .hidden { display: none; }
    </style>
    <script>
        function toggleMqtt() {
            const isMqtt = document.getElementById('mqtt_toggle').value === 'true';
            document.getElementById('mqtt_fields').className = isMqtt ? '' : 'hidden';
        }
    </script>
</head>
<body>
    <h1>🎄 Garland Setup</h1>
    <div class="container">
        <form action="/config" method="POST">
            <input name="ssid" placeholder="WiFi SSID" required>
            <input name="pass" type="password" placeholder="WiFi Password">
            
            <label>Mode:</label>
            <select name="mqtt_enabled" id="mqtt_toggle" onchange="toggleMqtt()">
                <option value="true">MQTT + Home Assistant</option>
                <option value="false">Local WiFi Only</option>
            </select>

            <div id="mqtt_fields">
                <input name="mqtt_url" placeholder="MQTT Broker IP" value="116.203.170.149">
                <input name="mqtt_user" placeholder="MQTT User" value="xmaslights">
                <input name="mqtt_pass" type="password" placeholder="MQTT Password">
            </div>

            <select name="color_order">
                <option value="RGB">RGB</option>
                <option value="GRB">GRB</option>
            </select>

            <button type="submit">Save & Restart</button>
        </form>
    </div>
</body>
</html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        config.wifiSSID = request->arg("ssid");
        config.wifiPassword = request->arg("pass");
        config.mqttEnabled = request->arg("mqtt_enabled") == "true";
        config.mqttServer = request->arg("mqtt_url");
        config.mqttUsername = request->arg("mqtt_user");
        config.mqttPassword = request->arg("mqtt_pass");
        config.colorOrder = request->arg("color_order");

        ConfigManager::saveConfig(config);
        request->send(200, "text/html", "<h1>Saved! Garland is restarting...</h1>");
        delay(2000);
        ESP.restart();
    });

    server.begin();
}

String getSsidWithChipId() {
    return "Garland-" + String(ESP.getChipId(), HEX);
}