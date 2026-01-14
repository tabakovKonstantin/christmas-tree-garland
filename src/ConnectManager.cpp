#include "ConnectManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <ESP8266WiFi.h>

extern LedControl ledControl;
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

    Serial.println("\nStarting Config Portal (AP Mode)...");
    ledControl.showError();

    WiFi.mode(WIFI_AP); 
    String apName = "Garland-" + String(ESP.getChipId(), HEX);
    WiFi.softAP(apName.c_str());
    
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    int n = WiFi.scanNetworks();

    server.on("/", HTTP_GET, [n](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Setup</title>
<link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🎄</text></svg>">
<style>
body {
  margin: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background: #0a0e14;
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 20px;
  color: #fff;
  overflow-x: hidden;
}
.snow { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 0; }
.card {
  position: relative; z-index: 1;
  background: rgba(255, 255, 255, 0.95);
  color: #333;
  border-radius: 20px;
  box-shadow: 0 10px 25px rgba(0,0,0,0.5);
  padding: 30px;
  max-width: 400px;
  width: 100%;
}
h1 { text-align: center; margin-top: 0; color: #c31432; font-size: 1.8rem; }
label { display: block; margin-top: 15px; margin-bottom: 5px; font-weight: 600; font-size: 0.9rem; color: #555; }
input, select {
  width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 8px;
  box-sizing: border-box; font-size: 1rem; background: #f9f9f9;
}
input:focus, select:focus { border-color: #c31432; outline: none; background: white; }
button {
  background: #27ae60; color: white; border: none; border-radius: 10px;
  font-size: 1.1rem; padding: 15px; width: 100%; margin-top: 25px;
  cursor: pointer; font-weight: bold; transition: transform 0.1s;
}
button:active { transform: scale(0.98); }
.hidden { display: none; }
.advanced-toggle {
    margin-top: 20px; text-align: center; color: #777; cursor: pointer;
    font-size: 0.9rem; text-decoration: underline;
}
.advanced-section { background: #f1f1f1; padding: 15px; border-radius: 10px; margin-top: 15px; }
.footer {
  font-size: 0.8rem; color: #888; margin-top: 20px; text-align: center;
}
</style>
<script>
 function toggleMqtt() {
  const val = document.getElementById('mqtt_toggle').value;
  document.getElementById('mqtt_fields').style.display = (val === 'true') ? 'block' : 'none';
 }
 function toggleAdvanced() {
     const el = document.getElementById('adv_fields');
     el.style.display = (el.style.display === 'none' || el.style.display === '') ? 'block' : 'none';
 }
</script>
</head>
<body>
<canvas class="snow" id="snowCanvas"></canvas>
<div class='card'>
  <h1>🎄 Setup</h1>
  <form action='/config' method='POST'>
    
    <label>🏠 Choose your Wi-Fi:</label>
    <select name='ssid'>)rawliteral";

        if (n <= 0) {
            html += "<option disabled selected>No networks found</option>";
        } else {
            for (int i = 0; i < n; ++i) {
                String s = WiFi.SSID(i);
                html += "<option value='" + htmlEscape(s) + "'>" + htmlEscape(s) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
            }
        }

        html += R"rawliteral(
    </select>
    
    <label>🔑 Wi-Fi Password:</label>
    <input type='password' name='pass' placeholder='Enter password'>

    <label>🎮 Control Mode:</label>
    <select name='mqtt_enabled' id='mqtt_toggle' onchange='toggleMqtt()'>
      <option value='false' selected>📱 Standalone (Web Interface)</option>
      <option value='true'>🏠 Home Assistant (MQTT)</option>
    </select>

    <div id='mqtt_fields' class='hidden' style='background: #fff3e0; padding: 10px; border-radius: 8px; margin-top:10px; border:1px solid #ffe0b2;'>
      <label>Broker Address:</label>
      <input name='mqtt_url' placeholder='192.168.1.x' value='116.203.170.149'>
      <div style="display:flex; gap:10px">
        <div style="flex:1"><label>User:</label><input name='mqtt_user' value='xmaslights'></div>
        <div style="flex:1"><label>Password:</label><input name='mqtt_pass' type='password'></div>
      </div>
    </div>

    <div class="advanced-toggle" onclick="toggleAdvanced()">🛠 Advanced Settings</div>
    <div id="adv_fields" class="hidden advanced-section">
        <label>🎨 LED Color Order:</label>
        <select name='color_order'>
            <option value='RGB'>RGB</option>
            <option value='GRB'>GRB</option>
            <option value='BRG'>BRG</option>
        </select>
    </div>

    <button type='submit'>🎅 Save & Start</button>
    <p class='footer'>Made with ❤️ for Christmas</p>
  </form>
</div>
<script>
toggleMqtt();
const canvas = document.getElementById('snowCanvas');
const ctx = canvas.getContext('2d');
let w, h, flakes = [];
function initSnow() {
    w = window.innerWidth; h = window.innerHeight;
    canvas.width = w; canvas.height = h;
    flakes = Array(50).fill().map(() => ({
        x: Math.random()*w, y: Math.random()*h, r: Math.random()*2+1, d: Math.random()+0.5
    }));
}
function draw() {
    ctx.clearRect(0,0,w,h);
    ctx.fillStyle = 'rgba(255,255,255,0.6)';
    ctx.beginPath();
    flakes.forEach(f => {
        ctx.moveTo(f.x, f.y); ctx.arc(f.x, f.y, f.r, 0, Math.PI*2);
        f.y += f.d; if(f.y > h) { f.y = -5; f.x = Math.random()*w; }
    });
    ctx.fill();
    requestAnimationFrame(draw);
}
window.onresize = initSnow;
initSnow(); draw();
</script>
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
        request->send(200, "text/html", "<h1 style='text-align:center; font-family:sans-serif; margin-top:50px;'>✅ Settings Saved!<br>Rebooting...</h1>");
        delay(1000);
        ESP.restart();
    });

    server.begin(); 
}

String getSsidWithChipId() {
    return "Garland-" + String(ESP.getChipId(), HEX);
}
void handleConfigRequest(AsyncWebServerRequest *request) {}