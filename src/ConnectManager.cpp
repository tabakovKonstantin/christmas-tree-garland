#include "ConnectManager.h"
#include "ConfigManager.h"
#include "LedControl.h"
#include <ESP8266WiFi.h>
#include <Updater.h>
#include <ArduinoJson.h>

extern LedControl ledControl;
Config config;
AsyncWebServer server(80);

// Pure HTML/JS/CSS - No server-side processing overhead
const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Setup</title>
<link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🎄</text></svg>">
<style>
body { margin: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background: #0a0e14; min-height: 100vh; display: flex; justify-content: center; align-items: center; padding: 20px; color: #fff; overflow-x: hidden; }
.snow { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 0; }
.card { position: relative; z-index: 1; background: rgba(255, 255, 255, 0.95); color: #333; border-radius: 20px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); padding: 30px; max-width: 400px; width: 100%; }
h1 { text-align: center; margin-top: 0; color: #c31432; font-size: 1.8rem; }
label { display: block; margin-top: 15px; margin-bottom: 5px; font-weight: 600; font-size: 0.9rem; color: #555; }
input, select { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 8px; box-sizing: border-box; font-size: 1rem; background: #f9f9f9; }
input:focus, select:focus { border-color: #c31432; outline: none; background: white; }
button { background: #27ae60; color: white; border: none; border-radius: 10px; font-size: 1.1rem; padding: 15px; width: 100%; margin-top: 25px; cursor: pointer; font-weight: bold; transition: transform 0.1s; }
button:active { transform: scale(0.98); }
.hidden { display: none; }
.advanced-toggle { margin-top: 20px; text-align: center; color: #777; cursor: pointer; font-size: 0.9rem; text-decoration: underline; }
.advanced-section { background: #f1f1f1; padding: 15px; border-radius: 10px; margin-top: 15px; }
.footer { font-size: 0.8rem; color: #888; margin-top: 20px; text-align: center; }
.pwd-wrap { position: relative; }
.pwd-toggle { position: absolute; right: 12px; top: 50%; transform: translateY(-50%); cursor: pointer; opacity: 0.5; display: flex; align-items: center; justify-content: center; }
.pwd-toggle:hover { opacity: 0.8; }
.pwd-toggle svg { width: 24px; height: 24px; fill: #666; }
select { appearance: none; -webkit-appearance: none; background-image: url("data:image/svg+xml;charset=US-ASCII,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%22292.4%22%20height%3D%22292.4%22%3E%3Cpath%20fill%3D%22%23FFFFFF%22%20d%3D%22M287%2069.4a17.6%2017.6%200%200%200-13-5.4H18.4c-5%200-9.3%201.8-12.9%205.4A17.6%2017.6%200%200%200%200%2082.2c0%205%201.8%209.3%205.4%2012.9l128%20127.9c3.6%203.6%207.8%205.4%2012.8%205.4s9.2-1.8%2012.8-5.4L287%2095c3.5-3.5%205.4-7.8%205.4-12.8%200-5-1.9-9.2-5.5-12.8z%22%2F%3E%3C%2Fsvg%3E"); background-repeat: no-repeat; background-position: right 15px top 50%; background-size: 12px auto; }
</style>
<script>
 function toggleMqtt() {
  const mode = document.getElementById('work_mode').value;
  document.getElementById('mqtt_fields').style.display = (mode === '1' || mode === '2') ? 'block' : 'none';
 }
 function toggleAdvanced() {
     const el = document.getElementById('adv_fields');
     el.style.display = (el.style.display === 'none' || el.style.display === '') ? 'block' : 'none';
 }
 function toggleVis(id) {
    var x = document.getElementById(id);
    if (x.type === "password") { x.type = "text"; } else { x.type = "password"; }
 }
 function validateForm(e) {
    const mode = document.getElementById('work_mode').value;
    if (mode === '0') return true; 
    var ip = document.getElementsByName('mqtt_url')[0].value;
    var ipRegex = /^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    if (!ipRegex.test(ip)) {
        alert("❌ Invalid MQTT Broker IP Address!\nPlease enter a valid IP (e.g., 192.168.1.10).");
        e.preventDefault();
        return false;
    }
    return true;
 }
 function uploadFw() {
    var input = document.getElementById('fw_file');
    if (!input.files.length) { alert("Please select a file first!"); return; }
    var file = input.files[0];
    var formData = new FormData();
    formData.append("firmware", file);
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/update", true);
    document.getElementById('progress_wrap').style.display = 'block';
    xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
            var percent = Math.round((e.loaded / e.total) * 100);
            document.getElementById('progress_bar').style.width = percent + '%';
            document.getElementById('progress_txt').innerText = percent + '%';
        }
    };
    xhr.onload = function() {
        if (xhr.status == 200) {
            document.getElementById('progress_txt').innerText = "Update Success! Rebooting...";
            document.getElementById('progress_bar').style.backgroundColor = "#2ecc71";
            setTimeout(function(){ location.reload(); }, 5000);
        } else {
            alert("Update Failed");
        }
    };
    xhr.send(formData);
 }
 
 async function loadData() {
    // Load Settings
    try {
        const resp = await fetch('/setup/settings');
        const data = await resp.json();
        
        document.getElementById('pass').value = data.wifiPassword || "";
        document.getElementById('work_mode').value = data.workMode;
        
        document.getElementById('mqtt_url').value = data.mqttServer || "116.203.170.149";
        document.getElementById('mqtt_port').value = data.mqttPort || 1883;
        document.getElementById('mqtt_user').value = data.mqttUsername || "";
        document.getElementById('mqtt_pass').value = data.mqttPassword || "";
        
        document.getElementById('color_order').value = data.colorOrder || "RGB";
        
        toggleMqtt();
    } catch(e) { console.error(e); }

    // Load Networks
    const ssidSel = document.getElementById('ssid');
    try {
        const resp = await fetch('/setup/scan');
        const nets = await resp.json();
        ssidSel.innerHTML = "";
        if(nets.length === 0) {
             let opt = document.createElement('option');
             opt.text = "No networks found (Rescan...)";
             ssidSel.add(opt);
             setTimeout(loadData, 5000); // Retry scan
        } else {
            nets.forEach(n => {
                let opt = document.createElement('option');
                opt.value = n.ssid;
                opt.text = n.ssid + " (" + n.rssi + " dBm)";
                ssidSel.add(opt);
            });
        }
    } catch(e) { 
        ssidSel.innerHTML = "<option>Scanning...</option>";
        setTimeout(loadData, 3000); 
    }
 }

 window.onload = function() {
    toggleMqtt();
    loadData();
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
 }
</script>
</head>
<body>
<canvas class="snow" id="snowCanvas"></canvas>
<div class='card'>
  <h1>🎄 Setup</h1>
  <form action='/config' method='POST' onsubmit='return validateForm(event)'>
    
    <label>🏠 Choose your Wi-Fi:</label>
    <select name='ssid' id='ssid'>
        <option disabled selected>Loading...</option>
    </select>
    
    <label>🔑 Wi-Fi Password:</label>
    <div class="pwd-wrap">
        <input type='password' id='pass' name='pass' placeholder='Enter password'>
        <span class="pwd-toggle" onclick="toggleVis('pass')">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
        </span>
    </div>

    <label>🎮 Control Mode:</label>
    <select name='work_mode' id='work_mode' onchange='toggleMqtt()'>
      <option value='0'>📱 Standalone (Web Interface)</option>
      <option value='1'>🤖 MQTT Only</option>
      <option value='2'>🚀 Both (Web + MQTT)</option>
    </select>

    <div id='mqtt_fields' class='hidden' style='background: #fff3e0; padding: 10px; border-radius: 8px; margin-top:10px; border:1px solid #ffe0b2;'>
      <div style="display:flex; gap:10px">
        <div style="flex:3">
            <label>Broker Address (IP):</label>
            <input name='mqtt_url' id='mqtt_url' placeholder='192.168.1.x'>
        </div>
        <div style="flex:1">
            <label>Port:</label>
            <input name='mqtt_port' id='mqtt_port' placeholder='1883'>
        </div>
      </div>

      <div style="display:flex; gap:10px">
        <div style="flex:1"><label>User:</label><input name='mqtt_user' id='mqtt_user'></div>
        <div style="flex:1">
            <label>Password:</label>
            <div class="pwd-wrap">
                <input name='mqtt_pass' id='mqtt_pass' type='password'>
                <span class="pwd-toggle" onclick="toggleVis('mqtt_pass')">
                    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
                </span>
            </div>
        </div>
      </div>
    </div>

    <div class="advanced-toggle" onclick="toggleAdvanced()">🛠 Advanced Settings</div>
    <div id="adv_fields" class="hidden advanced-section">
        <label>🎨 LED Color Order:</label>
        <select name='color_order' id='color_order'>
            <option value='RGB'>RGB</option>
            <option value='GRB'>GRB</option>
            <option value='BRG'>BRG</option>
        </select>

        <div style="border-top:1px solid #ddd; margin-top:20px; padding-top:15px;">
            <label>📡 Firmware Update:</label>
            <input type="file" id="fw_file" accept=".bin" style="background:white;">
            <button type="button" onclick="uploadFw()" style="background:#3498db; margin-top:10px;">🚀 Upload & Update</button>
            <div id="progress_wrap" style="display:none; margin-top:10px; background:#ddd; border-radius:5px; height:20px; overflow:hidden;">
                <div id="progress_bar" style="width:0%; height:100%; background:#27ae60; transition:width 0.2s;"></div>
            </div>
            <div id="progress_txt" style="text-align:center; font-size:0.8rem; margin-top:5px;"></div>
        </div>
    </div>

    <button type='submit'>🎅 Save & Start</button>
    <p class='footer'>Made with ❤️ for Christmas</p>
  </form>
</div>
</body>
</html>
)rawliteral";

void printWifiStatus() {
    Serial.print("Status: ");
    Serial.println(WiFi.status());
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
    WiFi.hostname(apName);
    WiFi.softAP(apName.c_str());
    
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

    // Start background scan immediately
    WiFi.scanNetworks(true); 

    // Serve static HTML (Zero RAM overhead)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", SETUP_HTML);
    });

    // JSON API for current settings
    server.on("/setup/settings", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["workMode"] = config.workMode;
        doc["wifiPassword"] = config.wifiPassword;
        doc["mqttServer"] = config.mqttServer;
        doc["mqttPort"] = config.mqttPort;
        doc["mqttUsername"] = config.mqttUsername;
        doc["mqttPassword"] = config.mqttPassword;
        doc["colorOrder"] = config.colorOrder;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // JSON API for Scan Results
    server.on("/setup/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanComplete();
        if(n == -2) {
            // Scan not triggered? Trigger it.
            WiFi.scanNetworks(true);
            request->send(200, "application/json", "[]");
        } else if(n == -1) {
            // Scanning...
            request->send(200, "application/json", "[]");
        } else {
            JsonDocument doc;
            for (int i = 0; i < n; ++i) {
                JsonObject net = doc.add<JsonObject>();
                net["ssid"] = WiFi.SSID(i);
                net["rssi"] = WiFi.RSSI(i);
            }
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
            // Re-trigger scan for next update
            WiFi.scanDelete();
            WiFi.scanNetworks(true);
        }
    });

    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        config.wifiSSID = request->arg("ssid");
        config.wifiPassword = request->arg("pass");
        config.workMode = request->arg("work_mode").toInt(); 
        config.mqttServer = request->arg("mqtt_url");
        int port = request->arg("mqtt_port").toInt();
        config.mqttPort = (port > 0) ? port : 1883;
        config.mqttUsername = request->arg("mqtt_user");
        config.mqttPassword = request->arg("mqtt_pass");
        config.colorOrder = request->arg("color_order");
        ConfigManager::saveConfig(config);
        request->send(200, "text/html", "<h1 style='text-align:center; font-family:sans-serif; margin-top:50px;'>✅ Settings Saved!<br>Rebooting...</h1>");
        delay(1000);
        ESP.restart();
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
        if (shouldReboot) {
            delay(500);
            ESP.restart();
        }
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            Serial.printf("Update Start: %s\n", filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if(!Update.begin(maxSketchSpace)){
                Update.printError(Serial);
            }
        }
        if(!Update.hasError()){
            if(Update.write(data, len) != len){
                Update.printError(Serial);
            }
        }
        if(final){
            if(Update.end(true)){
                Serial.printf("Update Success: %uB\n", index+len);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("http://" + WiFi.softAPIP().toString() + "/");
    });

    server.begin(); 
}

String getSsidWithChipId() {
    return "Garland-" + String(ESP.getChipId(), HEX);
}
void handleConfigRequest(AsyncWebServerRequest *request) {}