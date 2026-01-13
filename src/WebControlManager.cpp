#include "WebControlManager.h"
#include <ArduinoJson.h>

WebControlManager::WebControlManager(LedControl& led) : ledControl(led) {}

void WebControlManager::setup(AsyncWebServer& server) {
    // Main UI Page
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Garland Control</title>
    <style>
        body { background: #0a0e14; color: white; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; margin: 0; overflow-x: hidden; }
        .snow { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 0; }
        .container { position: relative; z-index: 1; max-width: 500px; margin: 0 auto; padding: 20px; }
        .card { background: rgba(255,255,255,0.1); border-radius: 16px; margin-bottom: 20px; padding: 20px; backdrop-filter: blur(10px); border: 1px solid rgba(255,255,255,0.1); box-shadow: 0 4px 30px rgba(0,0,0,0.5); }
        
        h1 { color: #f4d35e; text-shadow: 0 0 10px rgba(244,211,94,0.5); margin: 10px 0 30px; font-weight: 300; letter-spacing: 2px; }
        h3 { margin: 0 0 15px; font-weight: 400; color: #aaa; text-transform: uppercase; font-size: 0.9rem; letter-spacing: 1px; }

        /* Power Button */
        .pwr-btn {
            width: 80px; height: 80px; border-radius: 50%; border: none; outline: none; cursor: pointer;
            font-size: 24px; color: white; transition: all 0.3s ease;
            box-shadow: 0 0 15px rgba(0,0,0,0.5);
            background: #444; /* Default Unknown/Off */
        }
        .pwr-on { background: #2ecc71; box-shadow: 0 0 20px #2ecc71; }
        .pwr-off { background: #e74c3c; box-shadow: 0 0 20px #e74c3c; }

        /* Slider */
        .slider-container { display: flex; align-items: center; gap: 15px; }
        input[type=range] { flex-grow: 1; height: 6px; border-radius: 5px; background: #555; outline: none; -webkit-appearance: none; }
        input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #f4d35e; cursor: pointer; box-shadow: 0 0 10px #f4d35e; }
        .percent { font-size: 1.2rem; font-weight: bold; width: 50px; text-align: right; }

        /* Color Picker */
        input[type=color] { width: 100%; height: 50px; border: none; background: none; cursor: pointer; padding: 0; }

        /* Select Dropdown */
        select {
            width: 100%; padding: 15px; border-radius: 10px; border: 1px solid #555;
            background: #222; color: white; font-size: 1.1rem; outline: none; cursor: pointer;
            appearance: none; -webkit-appearance: none;
            background-image: url("data:image/svg+xml;charset=US-ASCII,%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20width%3D%22292.4%22%20height%3D%22292.4%22%3E%3Cpath%20fill%3D%22%23FFFFFF%22%20d%3D%22M287%2069.4a17.6%2017.6%200%200%200-13-5.4H18.4c-5%200-9.3%201.8-12.9%205.4A17.6%2017.6%200%200%200%200%2082.2c0%205%201.8%209.3%205.4%2012.9l128%20127.9c3.6%203.6%207.8%205.4%2012.8%205.4s9.2-1.8%2012.8-5.4L287%2095c3.5-3.5%205.4-7.8%205.4-12.8%200-5-1.9-9.2-5.5-12.8z%22%2F%3E%3C%2Fsvg%3E");
            background-repeat: no-repeat; background-position: right 15px top 50%; background-size: 12px auto;
        }
    </style>
</head>
<body>
    <canvas class="snow" id="snowCanvas"></canvas>
    <div class="container">
        <h1>🎄 Magic Garland</h1>
        
        <div class="card" style="text-align: center;">
            <button id="pwrBtn" class="pwr-btn pwr-on" onclick="togglePower()">
                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
            </button>
        </div>

        <div class="card">
            <h3>Brightness</h3>
            <div class="slider-container">
                <input type="range" id="bright" min="0" max="255" oninput="updateBriDisplay(this.value)" onchange="sendUpdate()">
                <span class="percent" id="briText">50%</span>
            </div>
        </div>

        <div class="card">
            <h3>Color</h3>
            <input type="color" id="color" value="#ff0000" onchange="sendUpdate()">
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
    </div>

    <script>
        let isPowerOn = true;

        function updateBriDisplay(val) {
            const pct = Math.round((val / 255) * 100);
            document.getElementById('briText').innerText = pct + "%";
        }

        function togglePower() {
            isPowerOn = !isPowerOn;
            updatePowerBtn();
            
            fetch('/api/set', {
                method: 'POST',
                body: JSON.stringify({ state: isPowerOn ? "ON" : "OFF" })
            });
        }

        function updatePowerBtn() {
            const btn = document.getElementById('pwrBtn');
            if (isPowerOn) {
                btn.classList.remove('pwr-off');
                btn.classList.add('pwr-on');
            } else {
                btn.classList.remove('pwr-on');
                btn.classList.add('pwr-off');
            }
        }

        function sendUpdate() {
            if (!isPowerOn) {
                isPowerOn = true;
                updatePowerBtn();
            }
            
            const b = document.getElementById('bright').value;
            const c = document.getElementById('color').value;
            const r = parseInt(c.substr(1,2), 16);
            const g = parseInt(c.substr(3,2), 16);
            const b_val = parseInt(c.substr(5,2), 16);

            // If we are changing color, force effect to null (Solid) in UI logic
            // But usually user selects effect manually. 
            // Let's just send color. If effect is active, hardware handles priority.
            
            fetch('/api/set', {
                method: 'POST',
                body: JSON.stringify({
                    state: "ON",
                    brightness: parseInt(b),
                    color: { r: r, g: g, b: b_val }
                })
            });
        }

        function setEffect(name) {
            if (!isPowerOn) {
                isPowerOn = true;
                updatePowerBtn();
            }
            fetch('/api/set', {
                method: 'POST',
                body: JSON.stringify({ state: "ON", effect: name })
            });
        }

        // Snow Animation
        const canvas = document.getElementById('snowCanvas');
        const ctx = canvas.getContext('2d');
        let width, height, flakes = [];
        function initSnow() {
            width = canvas.width = window.innerWidth;
            height = canvas.height = window.innerHeight;
            flakes = [];
            for(let i=0; i<80; i++) flakes.push({
                x: Math.random()*width, 
                y: Math.random()*height, 
                r: Math.random()*2+1, 
                d: Math.random()*1+0.5
            });
        }
        function drawSnow() {
            ctx.clearRect(0,0,width,height);
            ctx.fillStyle = 'rgba(255,255,255,0.6)';
            ctx.beginPath();
            flakes.forEach(f => {
                ctx.moveTo(f.x, f.y);
                ctx.arc(f.x, f.y, f.r, 0, Math.PI*2);
                f.y += f.d;
                if(f.y > height) { f.y = -5; f.x = Math.random()*width; }
            });
            ctx.fill();
            requestAnimationFrame(drawSnow);
        }
        window.onresize = initSnow;
        initSnow(); drawSnow();
    </script>
</body>
</html>
        )rawliteral";
        request->send(200, "text/html", html);
    });

    // API Endpoint
    server.on("/api/set", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            this->handleApiSet(request, data, len, index, total);
        }
    );
}

void WebControlManager::handleApiSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String payloadStr;
    for (size_t i = 0; i < len; i++) payloadStr += (char)data[i];
    
    Payload incoming;
    if (incoming.fromJson(payloadStr)) {
        ledControl.changeState(incoming);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(400, "application/json", "{\"status\":\"error\"}");
    }
}