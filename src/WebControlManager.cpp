#include "WebControlManager.h"
#include <ArduinoJson.h>
#include "ConfigManager.h"

// HTML Template
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Garland Control</title>
    <link rel="icon" href="data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🎄</text></svg>">
    <style>
        body { background: #0a0e14; color: white; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin: 0; overflow-x: hidden; }
        .snow { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 0; }
        .container { position: relative; z-index: 1; max-width: 500px; margin: 0 auto; padding: 20px; transition: filter 0.3s ease; }
        .card { background: rgba(255,255,255,0.1); border-radius: 16px; margin-bottom: 20px; padding: 20px; backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.1); box-shadow: 0 4px 30px rgba(0,0,0,0.5); }
        h1 { color: #f4d35e; text-shadow: 0 0 10px rgba(244,211,94,0.5); margin: 10px 0 30px; font-weight: 300; letter-spacing: 2px; }
        h3 { margin: 0 0 15px; font-weight: 400; color: #aaa; text-transform: uppercase; font-size: 0.9rem; letter-spacing: 1px; }
        
        .pwr-btn {
            width: 80px; height: 80px; border-radius: 50%; border: none; outline: none; cursor: pointer;
            font-size: 24px; color: white; transition: all 0.3s ease;
            box-shadow: 0 0 15px rgba(0,0,0,0.5);
            display: flex; align-items: center; justify-content: center; margin: 0 auto;
        }
        .pwr-on { background: #2ecc71; box-shadow: 0 0 20px #2ecc71; }
        .pwr-off { background: #e74c3c; box-shadow: 0 0 20px #e74c3c; }

        .slider-container { display: flex; align-items: center; gap: 15px; }
        input[type=range] { flex-grow: 1; height: 6px; border-radius: 5px; background: #555; outline: none; -webkit-appearance: none; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #f4d35e; cursor: pointer; box-shadow: 0 0 10px #f4d35e; }
        .percent { font-size: 1.2rem; font-weight: bold; width: 50px; text-align: right; }

        input[type=color] { width: 100%; height: 50px; border: none; background: none; cursor: pointer; padding: 0; }

        select {
            width: 100%; padding: 15px; border-radius: 10px; border: 1px solid #555;
            background: #222; color: white; font-size: 1.1rem; outline: none; cursor: pointer;
            appearance: none; -webkit-appearance: none;
            background-image: url("data:image/svg+xml;charset=US-ASCII,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%22292.4%22%20height%3D%22292.4%22%3E%3Cpath%20fill%3D%22%23FFFFFF%22%20d%3D%22M287%2069.4a17.6%2017.6%200%200%200-13-5.4H18.4c-5%200-9.3%201.8-12.9%205.4A17.6%2017.6%200%200%200%200%2082.2c0%205%201.8%209.3%205.4%2012.9l128%20127.9c3.6%203.6%207.8%205.4%2012.8%205.4s9.2-1.8%2012.8-5.4L287%2095c3.5-3.5%205.4-7.8%205.4-12.8%200-5-1.9-9.2-5.5-12.8z%22%2F%3E%3C%2Fsvg%3E");
            background-repeat: no-repeat; background-position: right 15px top 50%; background-size: 12px auto;
        }
        
        .action-link { display: block; margin-top: 30px; color: #888; text-decoration: none; font-size: 0.8rem; }

        /* Smooth Overlay Loader */
        .loader-overlay {
            position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            background: rgba(0, 0, 0, 0.4); 
            display: flex; align-items: center; justify-content: center;
            z-index: 9999;
            opacity: 0; pointer-events: none;
            transition: opacity 0.3s ease;
        }
        .loader-overlay.active { opacity: 1; pointer-events: auto; }
        
        /* Blur effect on container when loading */
        body.loading .container { filter: blur(4px); transform: scale(0.98); }

        .loader {
            border: 5px solid rgba(255,255,255,0.1); border-radius: 50%; border-top: 5px solid #f4d35e;
            width: 50px; height: 50px; animation: spin 0.8s linear infinite;
            box-shadow: 0 0 15px rgba(244, 211, 94, 0.4);
        }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div id="loader" class="loader-overlay"><div class="loader"></div></div>
    
    <canvas class="snow" id="snowCanvas"></canvas>
    
    <div class="container">
        <h1>🎄 Magic Garland</h1>
        
        <div class="card" style="text-align: center;">
            <button id="pwrBtn" class="pwr-btn %BTN_CLASS%" onclick="togglePower()">
                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
            </button>
        </div>

        <div class="card">
            <h3>Brightness</h3>
            <div class="slider-container">
                <input type="range" id="bright" min="0" max="255" value="%BRIGHT%" oninput="updateBriDisplay(this.value)" onchange="sendUpdate('bri')">
                <span class="percent" id="briText">%BRIGHT_PCT%%</span>
            </div>
        </div>

        <div class="card">
            <h3>Color</h3>
            <input type="color" id="color" value="#%HEX_COLOR%" onchange="sendUpdate('col')">
        </div>

        <div class="card">
            <h3>Effects</h3>
            <select id="effect" onchange="setEffect(this.value)">
                <option value="null">Solid Color (Stop Effect)</option>
                <option value="Rainbow">🌈 Rainbow</option>
                <option value="Smooth wave">🌊 Smooth Wave</option>
                <option value="Sparkle">✨ Sparkle</option>
                <option value="Tree">🌲 Christmas Tree</option>
                <option value="Halloween Flame">🔥 Fire / Flame</option>
            </select>
        </div>
        
        <a href="/reset_conf" class="action-link" onclick="return confirm('Reset WiFi settings?');">Reset Network Settings</a>
    </div>

    <script>
        let isPowerOn = %IS_PWR_ON%;

        function showLoader() { 
            document.getElementById('loader').classList.add('active');
            document.body.classList.add('loading');
        }
        function hideLoader() { 
            document.getElementById('loader').classList.remove('active');
            document.body.classList.remove('loading');
        }

        function updateBriDisplay(val) {
            const pct = Math.round((val / 255) * 100);
            document.getElementById('briText').innerText = pct + "%";
        }

        async function sendApi(data) {
            showLoader();
            try {
                const res = await fetch('/api/set', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(data)
                });
            } catch (e) { console.error(e); }
            // Small delay to make interaction feel smoother
            setTimeout(hideLoader, 150);
        }

        function togglePower() {
            isPowerOn = !isPowerOn;
            updatePowerBtn();
            sendApi({ state: isPowerOn ? "ON" : "OFF" });
        }

        function updatePowerBtn() {
            const btn = document.getElementById('pwrBtn');
            if (isPowerOn) {
                btn.classList.remove('pwr-off'); btn.classList.add('pwr-on');
            } else {
                btn.classList.remove('pwr-on'); btn.classList.add('pwr-off');
            }
        }

        function ensurePowerOn() {
            if (!isPowerOn) { isPowerOn = true; updatePowerBtn(); }
        }

        function sendUpdate(type) {
            ensurePowerOn();
            
            if (type === 'bri') {
                const b = parseInt(document.getElementById('bright').value);
                sendApi({ state: "ON", brightness: b });
            }
            else if (type === 'col') {
                const c = document.getElementById('color').value;
                const r = parseInt(c.substr(1,2), 16);
                const g = parseInt(c.substr(3,2), 16);
                const b_val = parseInt(c.substr(5,2), 16);
                
                document.getElementById('effect').value = 'null';
                
                sendApi({ 
                    state: "ON", 
                    color: { r: r, g: g, b: b_val },
                    effect: "null"
                });
            }
        }

        function setEffect(name) {
            ensurePowerOn();
            sendApi({ state: "ON", effect: name });
        }

        const canvas = document.getElementById('snowCanvas');
        const ctx = canvas.getContext('2d');
        let w, h, flakes = [];
        function initSnow() {
            w = window.innerWidth; h = window.innerHeight;
            canvas.width = w; canvas.height = h;
            flakes = Array(80).fill().map(() => ({
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
</html>
)rawliteral";

WebControlManager::WebControlManager(LedControl& led) : ledControl(led) {}

void WebControlManager::setup(AsyncWebServer& server) {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Payload st = this->ledControl.getCurrentState();
        bool isOn = (st.state == "ON");
        int r = (st.color.r < 0) ? 255 : st.color.r;
        int g = (st.color.g < 0) ? 0 : st.color.g;
        int b = (st.color.b < 0) ? 0 : st.color.b;
        int bright = (st.brightness < 0) ? 128 : st.brightness;

        char hexCol[7]; sprintf(hexCol, "%02x%02x%02x", r, g, b);

        String response = FPSTR(INDEX_HTML);
        response.replace("%IS_PWR_ON%", isOn ? "true" : "false");
        response.replace("%BTN_CLASS%", isOn ? "pwr-on" : "pwr-off");
        response.replace("%BRIGHT%", String(bright));
        response.replace("%BRIGHT_PCT%", String((int)(bright / 2.55)));
        response.replace("%HEX_COLOR%", String(hexCol));

        request->send(200, "text/html", response);
    });

    server.on("/api/set", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            static String jsonBuffer;
            if (index == 0) jsonBuffer = "";
            for (size_t i = 0; i < len; i++) jsonBuffer += (char)data[i];
            if (index + len == total) {
                Serial.print("API: "); Serial.println(jsonBuffer);
                Payload incoming;
                if (incoming.fromJson(jsonBuffer)) {
                    this->ledControl.changeState(incoming);
                }
            }
        }
    );
    
    server.on("/reset_conf", HTTP_GET, [](AsyncWebServerRequest *request){
        ConfigManager::eraseConfig();
        request->send(200, "text/html", "<h1>Resetting... connect to AP</h1><script>setTimeout(()=>{window.location.href='http://192.168.4.1'},5000)</script>");
        delay(1000);
        ESP.restart();
    });
}