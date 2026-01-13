#include "ConnectManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <ESP8266WiFi.h>

extern LedControl ledControl;

// Define the global variables here
Config config;
AsyncWebServer server(80);

static String htmlEscape(const String &input) {
    String s = input;
    s.replace("&", "&amp;"); s.replace("<", "&lt;"); s.replace(">", "&gt;");
    s.replace("\"", "&quot;"); s.replace("'", "&#39;");
    return s;
}

void printWifiStatus() {
    Serial.print("Status: ");
    switch (WiFi.status()) {
        case WL_IDLE_STATUS: Serial.println("IDLE"); break;
        case WL_NO_SSID_AVAIL: Serial.println("NO SSID"); break;
        case WL_SCAN_COMPLETED: Serial.println("SCAN CMPL"); break;
        case WL_CONNECTED: Serial.println("CONNECTED"); break;
        case WL_CONNECT_FAILED: Serial.println("FAILED"); break;
        case WL_CONNECTION_LOST: Serial.println("LOST"); break;
        case WL_DISCONNECTED: Serial.println("DISCONNECTED"); break;
        default: Serial.println("UNKNOWN"); break;
    }
}

void initWiFi() {
    Serial.println("Initializing WiFi...");
    
    WiFi.persistent(false);
    WiFi.disconnect(true);
    delay(200);
    
    bool hasConfig = ConfigManager::loadConfig(config);
    
    if (hasConfig && config.wifiSSID.length() > 0) {
        Serial.printf("Connecting to '%s'...\n", config.wifiSSID.c_str());
        
        WiFi.mode(WIFI_STA);
        String hostname = "Garland-" + String(ESP.getChipId(), HEX);
        WiFi.hostname(hostname);
        
        WiFi.begin(config.wifiSSID, config.wifiPassword);
        
        unsigned long start = millis();
        bool connected = false;
        
        // Wait up to 20 seconds
        while (millis() - start < 20000) {
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                break;
            }
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        if (connected) {
            Serial.println("WiFi Connected!");
            Serial.print("IP: "); Serial.println(WiFi.localIP());
            ledControl.showSuccess();
            return; 
        } else {
            Serial.print("Connection Failed! ");
            printWifiStatus();
        }
    }

    // --- FALLBACK TO AP MODE ---
    Serial.println("\nStarting Config Portal (AP Mode)...");
    ledControl.showError();

    WiFi.mode(WIFI_AP); 
    String apName = "Garland-" + String(ESP.getChipId(), HEX);
    WiFi.softAP(apName.c_str());
    
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    // START CONFIG PORTAL HANDLERS
    int n = WiFi.scanNetworks();

    server.on("/", HTTP_GET, [n](AsyncWebServerRequest *request) {
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
  width: 100%;
}
button:hover {
  background: #27ae60;
  transform: scale(1.05);
}
.hidden { display: none; }
.footer {
  font-size: 0.9rem;
  color: #666;
  margin-top: 10px;
}
</style>
<script>
 function toggleMqtt() {
  const isMqtt = document.getElementById('mqtt_toggle').value === 'true';
  document.getElementById('mqtt_section').style.display = isMqtt ? 'block' : 'none';
 }
</script>
</head>
<body>
<div class='card'>
<h1>🎄 Wi-Fi & MQTT Setup</h1>
<form action='/config' method='POST'>
<div class='section'>
<h3>📶 Wi-Fi</h3>
<label for='ssid'>Select SSID:</label>
<select id='ssid' name='ssid'>)rawliteral";

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

        html += R"rawliteral(
</select>
<label for='password'>Password:</label>
<input id='password' type='password' name='pass' placeholder='Wi-Fi Password'>
</div>

<div class='section'>
<h3>⚙️ Operation Mode</h3>
<select name='mqtt_enabled' id='mqtt_toggle' onchange='toggleMqtt()'>
 <option value='true'>MQTT + Web (Home Assistant)</option>
 <option value='false'>Local Web Mode Only</option>
</select>
</div>

<div class='section' id='mqtt_section'>
<h3>🔌 MQTT Settings</h3>
<label for='mqtt_url'>Server URL:</label>
<input id='mqtt_url' type='text' name='mqtt_url' placeholder='e.g. 192.168.1.10' value='116.203.170.149'>

<div style='display:flex; gap:10px;'>
  <div style='flex:2;'>
    <label for='mqtt_user'>Username:</label>
    <input id='mqtt_user' type='text' name='mqtt_user' placeholder='user' value='xmaslights'>
  </div>
  <div style='flex:1;'>
    <label for='mqtt_pass'>Pass:</label>
    <input id='mqtt_pass' type='password' name='mqtt_pass' placeholder='pass'>
  </div>
</div>
</div>

<div class='section'>
<h3>💡 LED Order</h3>
<select name='color_order'>
  <option value='RGB'>RGB</option>
  <option value='GRB'>GRB</option>
  <option value='BRG'>BRG</option>
</select>
</div>

<button type='submit'>🎅 Save & Restart</button>
<p class='footer'>Made with ❤️ for Christmas</p>
</form>
</div>
<script>toggleMqtt();</script>
</body>
</html>)rawliteral";

        request->send(200, "text/html", html);
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        config.wifiSSID = request->arg("ssid");
        config.wifiPassword = request->arg("pass");
        config.mqttEnabled = (request->arg("mqtt_enabled") == "true");
        config.mqttServer = request->arg("mqtt_url");
        config.mqttUsername = request->arg("mqtt_user");
        config.mqttPassword = request->arg("mqtt_pass");
        config.colorOrder = request->arg("color_order");
        
        ConfigManager::saveConfig(config);
        request->send(200, "text/html", "<h1>Saved! Rebooting...</h1>");
        delay(1000);
        ESP.restart();
    });

    server.begin(); 
}

String getSsidWithChipId() {
    return "Garland-" + String(ESP.getChipId(), HEX);
}
void handleConfigRequest(AsyncWebServerRequest *request) {}