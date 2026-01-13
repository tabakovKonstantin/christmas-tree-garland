#include "WebControlManager.h"
#include <ArduinoJson.h>

WebControlManager::WebControlManager(LedControl& led) : ledControl(led) {}

void WebControlManager::setup(AsyncWebServer& server) {
    // Main UI Page
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Garland Control</title>
    <style>
        body { background: #0a0e14; color: white; font-family: sans-serif; text-align: center; margin: 0; overflow-x: hidden; }
        .snow { position: fixed; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; z-index: 100; }
        .card { background: rgba(255,255,255,0.1); border-radius: 20px; margin: 20px; padding: 20px; backdrop-filter: blur(10px); }
        input[type=range] { width: 80%; height: 15px; border-radius: 5px; background: #d3d3d3; outline: none; margin: 20px 0; }
        .btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; padding: 10px; }
        button { padding: 15px; border-radius: 10px; border: none; background: #c91c24; color: white; font-weight: bold; cursor: pointer; }
        button:active { transform: scale(0.95); }
        #colorPicker { width: 150px; height: 150px; border: none; background: none; cursor: pointer; }
        h1 { color: #f4d35e; text-shadow: 0 0 10px rgba(244,211,94,0.5); margin-top: 30px; }
    </style>
</head>
<body>
    <canvas class="snow" id="snowCanvas"></canvas>
    <h1>🎄 Magic Garland</h1>
    <div class="card">
        <h3>Brightness</h3>
        <input type="range" id="bright" min="0" max="255" onchange="sendUpdate()">
    </div>
    <div class="card">
        <h3>Color</h3>
        <input type="color" id="color" onchange="sendUpdate()">
    </div>
    <div class="card">
        <h3>Effects</h3>
        <div class="btn-grid">
            <button onclick="setEffect('Rainbow')">🌈 Rainbow</button>
            <button onclick="setEffect('Smooth wave')">🌊 Wave</button>
            <button onclick="setEffect('Sparkle')">✨ Sparkle</button>
            <button onclick="setEffect('Tree')">🌲 Tree</button>
            <button onclick="setEffect('Halloween Flame')">🔥 Flame</button>
            <button onclick="setEffect('null')">⏹ Stop</button>
        </div>
    </div>

    <script>
        function sendUpdate() {
            const b = document.getElementById('bright').value;
            const c = document.getElementById('color').value;
            const r = parseInt(c.substr(1,2), 16);
            const g = parseInt(c.substr(3,2), 16);
            const b_val = parseInt(c.substr(5,2), 16);

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
            fetch('/api/set', {
                method: 'POST',
                body: JSON.stringify({ state: "ON", effect: name })
            });
        }

        // Simple Snow effect
        const canvas = document.getElementById('snowCanvas');
        const ctx = canvas.getContext('2d');
        let width, height, flakes = [];
        function initSnow() {
            width = canvas.width = window.innerWidth;
            height = canvas.height = window.innerHeight;
            flakes = [];
            for(let i=0; i<100; i++) flakes.push({x: Math.random()*width, y: Math.random()*height, r: Math.random()*3+1, d: Math.random()+0.5});
        }
        function drawSnow() {
            ctx.clearRect(0,0,width,height);
            ctx.fillStyle = 'white';
            ctx.beginPath();
            flakes.forEach(f => {
                ctx.moveTo(f.x, f.y);
                ctx.arc(f.x, f.y, f.r, 0, Math.PI*2);
                f.y += f.d;
                if(f.y > height) f.y = -10;
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