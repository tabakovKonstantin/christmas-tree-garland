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

// Helper to print connection status
void printWifiStatus() {
    switch (WiFi.status()) {
        case WL_IDLE_STATUS: Serial.print(" IDLE"); break;
        case WL_NO_SSID_AVAIL: Serial.print(" NO SSID"); break;
        case WL_SCAN_COMPLETED: Serial.print(" SCAN CMPL"); break;
        case WL_CONNECTED: Serial.print(" CNCTD"); break;
        case WL_CONNECT_FAILED: Serial.print(" FAIL"); break;
        case WL_CONNECTION_LOST: Serial.print(" LOST"); break;
        case WL_DISCONNECTED: Serial.print(" DISC"); break;
        default: Serial.print(" UNKNOWN"); break;
    }
}

void initWiFi() {
    Serial.println("Initializing WiFi...");
    
    // 1. Force cleanup of previous state
    WiFi.persistent(false); // Don't save credentials to Flash (prevents conflicts)
    WiFi.disconnect(true);  // Disconnect and turn off radio
    delay(200);
    
    bool hasConfig = ConfigManager::loadConfig(config);
    
    if (hasConfig && config.wifiSSID.length() > 0) {
        Serial.printf("Connecting to '%s'...\n", config.wifiSSID.c_str());
        
        // 2. Set mode and Hostname BEFORE begin
        WiFi.mode(WIFI_STA);
        String hostname = "Garland-" + String(ESP.getChipId(), HEX);
        WiFi.hostname(hostname);
        
        // 3. Begin connection
        WiFi.begin(config.wifiSSID, config.wifiPassword);
        
        // 4. Wait up to 35 seconds with detailed status feedback
        unsigned long start = millis();
        bool connected = false;
        
        while (millis() - start < 35000) {
            if (WiFi.status() == WL_CONNECTED) {
                connected = true;
                break;
            }
            delay(500);
            Serial.print(".");
            // Occasionally print status code for debugging
            if ((millis() / 500) % 10 == 0) printWifiStatus(); 
        }
        
        Serial.println();
        
        if (connected) {
            Serial.println("WiFi Connected!");
            Serial.print("STA IP: "); Serial.println(WiFi.localIP());
            Serial.print("Hostname: "); Serial.println(hostname);
            Serial.print("Signal: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
            ledControl.showSuccess();
            return;
        } else {
            Serial.print("WiFi Connection Failed! Final Status:");
            printWifiStatus();
            Serial.println();
            Serial.println("Possible reasons: Wrong Password, 5GHz only (ESP needs 2.4GHz), or WPA3.");
        }
    } else {
        Serial.println("No WiFi config found.");
    }

    // --- FALLBACK TO AP MODE ---
    Serial.println("\nStarting Config Portal (AP Mode)...");
    ledControl.showError(); // Red flash indicates AP mode

    // Scan networks for UI (blocking)
    WiFi.mode(WIFI_STA); 
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    
    // Setup AP
    WiFi.mode(WIFI_AP);
    String apName = "Garland-" + String(ESP.getChipId(), HEX);
    WiFi.softAP(apName.c_str());
    
    Serial.print("AP Name: "); Serial.println(apName);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    // Setup Web Server for Config
    server.on("/", HTTP_GET, [n](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
<style>
 body { font-family: sans-serif; background: #c91c24; color: white; text-align: center; margin: 0; padding: 20px; }
 .card { background: white; color: #333; padding: 20px; border-radius: 15px; max-width: 400px; margin: auto; box-shadow: 0 4px 10px rgba(0,0,0,0.3); }
 input, select { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ccc; border-radius: 8px; box-sizing: border-box; }
 button { background: #2ecc71; color: white; border: none; padding: 15px; width: 100%; border-radius: 8px; font-weight: bold; cursor: pointer; }
 .hidden { display: none; }
 h2 { color: #c91c24; }
</style>
<script>
 function toggleMqtt() {
  const isMqtt = document.getElementById('mqtt_toggle').value === 'true';
  document.getElementById('mqtt_fields').style.display = isMqtt ? 'block' : 'none';
 }
</script></head><body>
<h1>🎄 Setup</h1><div class='card'><h2>Network Settings</h2>
<form action='/config' method='POST'>
<label>WiFi Network:</label>
<select name='ssid' id='ssid_select'>)rawliteral";

        if (n <= 0) {
            html += "<option value=''>No networks found</option>";
        } else {
            for (int i = 0; i < n; ++i) {
                String s = WiFi.SSID(i);
                html += "<option value='" + htmlEscape(s) + "'>" + htmlEscape(s) + " (" + String(WiFi.RSSI(i)) + "dBm)</option>";
            }
        }

        html += R"rawliteral(</select>
<input name='pass' type='password' placeholder='WiFi Password'>
<hr>
<label>Operation Mode:</label>
<select name='mqtt_enabled' id='mqtt_toggle' onchange='toggleMqtt()'>
 <option value='true'>MQTT + Web (Home Assistant)</option>
 <option value='false'>Local Web Mode Only</option>
</select>
<div id='mqtt_fields'>
 <input name='mqtt_url' placeholder='Broker IP' value='116.203.170.149'>
 <input name='mqtt_user' placeholder='User' value='xmaslights'>
 <input name='mqtt_pass' type='password' placeholder='Password'>
</div>
<label>LED Order:</label>
<select name='color_order'>
 <option value='RGB'>RGB</option><option value='GRB'>GRB</option><option value='BRG'>BRG</option>
</select>
<button type='submit'>🎅 Save & Restart</button>
</form></div></body></html>)rawliteral";
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
        request->send(200, "text/html", "<h2>Settings Saved! Garland is rebooting...</h2>");
        delay(2000);
        ESP.restart();
    });

    server.begin();
}

String getSsidWithChipId() {
    return "Garland-" + String(ESP.getChipId(), HEX);
}
void handleConfigRequest(AsyncWebServerRequest *request) {
    // moved to lambda
}