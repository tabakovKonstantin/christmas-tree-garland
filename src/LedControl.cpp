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
    }
    
    FastLED.show();
}

void LedControl::eventFlash()
{
    if (eventStep >= eventTotalFlashes * 2) {
        if (!isOn) fill_solid(leds, NUM_LEDS, CRGB::Black);
        else {
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
        // If color is set, we usually stop effect, BUT
        // we wait to see if 'effect' field is also present.
        // If effect is NOT present, we stop it.
        // If effect IS present, it overrides color.
        if (payload.effect.length() == 0) {
             next.effect = "null"; 
             effectManager.setEffect(nullptr);
        }
    }

    // Effect
    // payload.effect == "" -> No change
    // payload.effect == "null" -> Stop
    // payload.effect == "Name" -> Start
    if (payload.effect.length() > 0) {
        if (payload.effect == "null") {
            effectManager.setEffect(nullptr);
            next.effect = "null";
        } else {
            setLEDEffect(payload.effect);
            next.effect = payload.effect;
        }
    }

    // Power State
    if (payload.state == "OFF") {
        FastLED.clear(true);
        isOn = false;
        next.state = "OFF";
    } else if (payload.state == "ON") {
        isOn = true;
        next.state = "ON";
    }

    // --- 2. RE-APPLY VISUALS IF NEEDED ---
    // If we are ON, make sure something is showing
    if (isOn) {
        // If we just turned ON, or if we are ON and just changed something
        if (next.effect != "null" && next.effect.length() > 0) {
            // Ensure effect is running (setEffect handles duplicates internally usually, 
            // but EffectManager::setEffect re-creates it. 
            // We only re-set if it changed OR if we were off.
            // Simplified: If effect is active in 'next', just ensure it runs.
            // Optimally: check if changed.
            // For now, if we have an effect in 'next', we rely on the fact 
            // that we called setLEDEffect above if it was in payload.
            // If it wasn't in payload, it might be stopped if we were OFF.
            // So if (wasOff) restart it.
            // But we don't track 'wasOff' easily here without extra var.
            // Let's just trust setLEDEffect was called if payload had it.
            
            // If payload DID NOT have effect, but next HAS effect, and we are ON...
            // We might be just changing brightness on a running effect.
            // In that case, do nothing (brightness applied above).
        } else {
            // Solid Color mode
            // If we changed color, we called setLEDColor above.
            // If we just changed brightness, we need to redraw solid color?
            // FastLED.setBrightness handles scaling on show(), 
            // but we need to re-push the color to array if it was cleared?
            // No, FastLED array keeps values. But if we were OFF, it was cleared.
            // So we should re-apply color.
            uint32_t rgb = (next.color.r << 16) | (next.color.g << 8) | next.color.b;
            fill_solid(leds, NUM_LEDS, CRGB(rgb));
            FastLED.show();
        }
    }

    currentState = next;
    StateStorage::saveState(currentState);
}

void LedControl::setLEDColor(uint32_t color) { 
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