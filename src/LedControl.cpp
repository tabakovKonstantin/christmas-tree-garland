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
        if (currentState.effect != "null") {
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
        else changeState(currentState); // Restore previous visuals
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
    bool hasBrightness = payload.brightness != -1;
    bool hasColorRGB = (payload.color.r != -1 && payload.color.g != -1 && payload.color.b != -1);
    bool hasEffect = (payload.effect != "null" && payload.effect.length() > 0);

    if (hasBrightness) {
        setLEDBrightness(payload.brightness);
        next.brightness = payload.brightness;
    }

    if (hasColorRGB) {
        uint32_t rgb = (payload.color.r << 16) | (payload.color.g << 8) | payload.color.b;
        setLEDColor(rgb);
        next.color = payload.color;
        next.effect = "null";
        effectManager.setEffect(nullptr);
    }

    if (hasEffect) {
        setLEDEffect(payload.effect);
        next.effect = payload.effect;
    } else if (payload.effect == "null") {
        effectManager.setEffect(nullptr);
        next.effect = "null";
    }

    if (payload.state == "OFF") {
        FastLED.clear(true);
        isOn = false;
        next.state = "OFF";
    } else if (payload.state == "ON") {
        isOn = true;
        next.state = "ON";
    }

    currentState = next;
    // Debounced save could be added here, for now direct save
    StateStorage::saveState(currentState);
}

void LedControl::setLEDColor(uint32_t color) { fill_solid(leds, NUM_LEDS, CRGB(color)); FastLED.show(); }
void LedControl::setLEDBrightness(int brightness) { FastLED.setBrightness(brightness); FastLED.show(); }
void LedControl::setLEDEffect(String effect) {
    Effect *eff = EffectFactory::createEffect(effect);
    effectManager.setEffect(eff);
    effectManager.runEffect(leds, NUM_LEDS);
}

const Payload &LedControl::getCurrentState() const { return currentState; }