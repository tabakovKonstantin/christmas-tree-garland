#include "LedControl.h"
#include "effect/EffectFactory.h"
#include "ConfigManager.h"
#include "StateStorage.h"

LedControl::LedControl(EffectManager &manager) : effectManager(manager)
{
    // Try to load saved state
    if (!StateStorage::loadState(currentState)) {
        currentState.brightness = BRIGHTNESS;
        currentState.color.r = 255;
        currentState.color.g = 200;
        currentState.color.b = 120;
        currentState.color_mode = "rgb";
        currentState.effect = "null";
        currentState.state = "OFF";
    }
}

void LedControl::initLEDs()
{
    Serial.println("Initializing LEDs...");
    delay(1000);

    String orderName = "RGB";
    Config cfg;
    if (ConfigManager::loadConfig(cfg)) {
        if (cfg.colorOrder.length() > 0) orderName = cfg.colorOrder;
    }

    if (orderName.equalsIgnoreCase("RGB"))
        FastLED.addLeds<LED_TYPE, LED_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else if (orderName.equalsIgnoreCase("RBG"))
        FastLED.addLeds<LED_TYPE, LED_PIN, RBG>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else if (orderName.equalsIgnoreCase("GRB"))
        FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else if (orderName.equalsIgnoreCase("GBR"))
        FastLED.addLeds<LED_TYPE, LED_PIN, GBR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else if (orderName.equalsIgnoreCase("BRG"))
        FastLED.addLeds<LED_TYPE, LED_PIN, BRG>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else if (orderName.equalsIgnoreCase("BGR"))
        FastLED.addLeds<LED_TYPE, LED_PIN, BGR>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    else
        FastLED.addLeds<LED_TYPE, LED_PIN, RGB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    FastLED.setDither(false);
    FastLED.setBrightness(currentState.brightness);
    
    // Apply initial state
    if (currentState.state == "ON") {
        isOn = true;
        if (currentState.effect != "null" && currentState.effect.length() > 0) {
            setLEDEffect(currentState.effect);
        } else {
            setLEDColor((currentState.color.r << 16) | (currentState.color.g << 8) | currentState.color.b);
        }
    } else {
        FastLED.clear(true);
        isOn = false;
        // Make sure no effect is running on boot if OFF
        effectManager.setEffect(nullptr);
    }
    
    FastLED.show();
}

void LedControl::eventFlash()
{
    if (eventStep >= eventTotalFlashes * 2) {
        if (!isOn) {
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            effectManager.setEffect(nullptr); // Ensure stopped
        } else {
            // Restore visuals
            if (currentState.effect != "null" && currentState.effect.length() > 0) {
                 setLEDEffect(currentState.effect);
            } else {
                 uint32_t rgb = (currentState.color.r << 16) | (currentState.color.g << 8) | currentState.color.b;
                 setLEDColor(rgb);
            }
        }
        FastLED.show();
        eventTicker.detach();
        return;
    }
    // Temporarily pause effect during flash? Ideally yes.
    effectManager.setEffect(nullptr);
    
    if (eventStep % 2 == 0) fill_solid(leds, NUM_LEDS, eventColor);
    else fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    eventStep++;
}

void LedControl::showEventNotification(const CRGB &color, int flashes, int delayMs)
{
    eventTicker.detach();
    eventStep = 0;
    eventTotalFlashes = flashes;
    eventColor = color;
    eventTicker.attach_ms(delayMs, [this]() { this->eventFlash(); });
}

void LedControl::showSuccess() { showEventNotification(CRGB::Green, 3, 300); }
void LedControl::showError() { showEventNotification(CRGB::Red, 6, 600); }

void LedControl::changeState(const Payload &payload)
{
    Payload next = currentState;
    bool wasOff = !isOn;
    
    // --- 1. PARSE & APPLY PARAMS ---
    
    // Brightness
    if (payload.brightness != -1) {
        setLEDBrightness(payload.brightness);
        next.brightness = payload.brightness;
    }

    // Color (Implicitly stops effect if set)
    bool colorChanged = (payload.color.r != -1);
    if (colorChanged) {
        next.color = payload.color;
        // If color is set, we check if effect is explicitly kept
        if (payload.effect.length() == 0) {
             next.effect = "null"; 
             // We stop effect later if ON
        }
    }

    // Effect Logic
    bool effectChanged = false;
    if (payload.effect.length() > 0) {
        if (payload.effect == "null") {
            next.effect = "null";
        } else {
            next.effect = payload.effect;
            effectChanged = true;
        }
    }

    // Power State Logic
    if (payload.state == "OFF") {
        // TURN OFF
        isOn = false;
        next.state = "OFF";
        
        // CRITICAL FIX: Stop any running effect so it doesn't overwrite black
        effectManager.setEffect(nullptr);
        FastLED.clear(true);
        FastLED.show();
        
    } else if (payload.state == "ON") {
        // TURN ON
        isOn = true;
        next.state = "ON";
    }

    // --- 2. RE-APPLY VISUALS IF ON ---
    if (isOn) {
        // Did we just turn on? Or did effect change?
        bool needRestartEffect = (wasOff || effectChanged);
        
        if (next.effect != "null" && next.effect.length() > 0) {
            // Active Effect
            if (needRestartEffect) {
                setLEDEffect(next.effect);
            }
            // If just brightness changed, effect continues running fine.
        } else {
            // Solid Color
            // Stop effect if it was running
            effectManager.setEffect(nullptr);
            
            uint32_t rgb = (next.color.r << 16) | (next.color.g << 8) | next.color.b;
            fill_solid(leds, NUM_LEDS, CRGB(rgb));
            FastLED.show();
        }
    }

    currentState = next;
    StateStorage::saveState(currentState);
}

void LedControl::setLEDColor(uint32_t color) { 
    // Stop any effect before setting solid color
    effectManager.setEffect(nullptr);
    fill_solid(leds, NUM_LEDS, CRGB(color)); 
    FastLED.show(); 
}

void LedControl::setLEDBrightness(int brightness) { 
    FastLED.setBrightness(brightness); 
    FastLED.show(); 
}

void LedControl::setLEDEffect(String effect) {
    Effect *eff = EffectFactory::createEffect(effect);
    effectManager.setEffect(eff);
    effectManager.runEffect(leds, NUM_LEDS);
}

const Payload &LedControl::getCurrentState() const { return currentState; }