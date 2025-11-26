#include "LedControl.h"
#include "effect/EffectFactory.h"
#include "ConfigManager.h"

// -------------------------------------------------------
//  КОНСТРУКТОР
// -------------------------------------------------------
LedControl::LedControl(EffectManager &manager) : effectManager(manager)
{
    // Инициализация дефолта состояния
    currentState.brightness = BRIGHTNESS;

    currentState.color.r = -1;
    currentState.color.g = -1;
    currentState.color.b = -1;
    currentState.color.c = -1;
    currentState.color.w = -1;

    currentState.color_mode = "rgb";  // константа

    currentState.effect = "null";
    currentState.state = "OFF";
    currentState.transition = 0;
}

// -------------------------------------------------------
//  ИНИЦИАЛИЗАЦИЯ СВЕТОДИОДОВ
// -------------------------------------------------------
void LedControl::initLEDs()
{
    Serial.println("Initializing LEDs...");
    delay(1000);

    // Читаем конфиг для цветового порядка
    String orderName = "RGB";
    Config cfg;

    if (ConfigManager::loadConfig(cfg))
    {
        if (cfg.colorOrder.length() > 0)
        {
            orderName = cfg.colorOrder;
        }
    }

    // Выбор LED порядка (шаблон FastLED требует compile-time параметров)
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
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear(true);
    FastLED.show();

    isOn = false;
}

// -------------------------------------------------------
//  EVENT FLASH / SUCCESS / ERROR
// -------------------------------------------------------
void LedControl::eventFlash()
{
    if (eventStep >= eventTotalFlashes * 2)
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
        eventTicker.detach();
        return;
    }

    if (eventStep % 2 == 0)
        fill_solid(leds, NUM_LEDS, eventColor);
    else
        fill_solid(leds, NUM_LEDS, CRGB::Black);

    FastLED.show();
    eventStep++;
}

void LedControl::showEventNotification(const CRGB &color, int flashes, int delayMs)
{
    eventTicker.detach();
    eventStep = 0;
    eventTotalFlashes = flashes;
    eventColor = color;

    eventTicker.attach_ms(delayMs, [this]()
                          { this->eventFlash(); });
}

void LedControl::showSuccess()
{
    showEventNotification(CRGB::Green, 3, 300);
}

void LedControl::showError()
{
    showEventNotification(CRGB::Red, 6, 600);
}

// -------------------------------------------------------
//  DEFAULT WARM LOGIC
// -------------------------------------------------------
bool LedControl::shouldUseWarmDefault(const Payload &payload,
                                      bool hasBrightness,
                                      bool hasColorRGB,
                                      bool hasEffect,
                                      bool isCurrentlyOn) const
{
    if (!hasBrightness)
        return false;

    if (hasColorRGB || hasEffect)
        return false;

    bool hasState = payload.state.length() > 0;
    bool stateOn = payload.state.equalsIgnoreCase("ON");

    if (isCurrentlyOn)
        return false;

    if (!hasState)
        return true;

    if (stateOn)
        return true;

    return false;
}

void LedControl::applyWarmDefault(int brightness)
{
    Serial.println("Applying warm yellow default state.");

    setLEDBrightness(brightness);

    CRGB warmYellow(255, 200, 120);
    fill_solid(leds, NUM_LEDS, warmYellow);
    FastLED.show();

    isOn = true;
}

// -------------------------------------------------------
//  CHANGE STATE
// -------------------------------------------------------
void LedControl::changeState(const Payload &payload)
{
    Serial.println("Applying new state...");

    Payload next = currentState;

    bool hasBrightness = payload.brightness != -1;
    bool hasColorRGB = (payload.color.r != -1 &&
                        payload.color.g != -1 &&
                        payload.color.b != -1);
    bool hasEffect = (payload.effect != "null" && payload.effect.length() > 0);

    bool useWarm = shouldUseWarmDefault(payload, hasBrightness, hasColorRGB, hasEffect, isOn);

    if (useWarm)
    {
        int brightness = hasBrightness ? payload.brightness : next.brightness;

        applyWarmDefault(brightness);

        next.brightness = brightness;
        next.state = "ON";
        next.color.r = 255;
        next.color.g = 200;
        next.color.b = 120;
        next.effect = "null";
    }
    else
    {
        if (hasBrightness)
        {
            setLEDBrightness(payload.brightness);
            next.brightness = payload.brightness;
        }

        if (hasColorRGB)
        {
            uint32_t rgb = (payload.color.r << 16) |
                           (payload.color.g << 8) |
                           payload.color.b;

            setLEDColor(rgb);

            next.color.r = payload.color.r;
            next.color.g = payload.color.g;
            next.color.b = payload.color.b;
        }

        if (hasEffect)
        {
            setLEDEffect(payload.effect);
            next.effect = payload.effect;
        }
        else if (payload.brightness == -1)
        {
            effectManager.setEffect(nullptr);
            next.effect = "null";
        }

        if (payload.state.equalsIgnoreCase("OFF"))
        {
            FastLED.clear();
            isOn = false;
            next.state = "OFF";
        }
        else if (payload.state.equalsIgnoreCase("ON"))
        {
            if (!hasColorRGB && payload.effect == "null" && payload.brightness == -1)
            {
                fill_solid(leds, NUM_LEDS, CRGB::White);
                next.color.r = 255;
                next.color.g = 255;
                next.color.b = 255;
            }
            isOn = true;
            next.state = "ON";
        }
    }

    if (payload.transition != 0)
        next.transition = payload.transition;

    next.color_mode = "rgb";

    currentState = next;

    Serial.println("Applying has finished");
}

// -------------------------------------------------------
//  SETTERS
// -------------------------------------------------------
void LedControl::setLEDColor(uint32_t color)
{
    fill_solid(leds, NUM_LEDS, CRGB(color));
}

void LedControl::setLEDBrightness(int brightness)
{
    FastLED.setBrightness(brightness);
}

void LedControl::setLEDEffect(String effect)
{
    Effect *eff = EffectFactory::createEffect(effect);
    effectManager.setEffect(eff);
    effectManager.runEffect(leds, NUM_LEDS);
}

const Payload &LedControl::getCurrentState() const
{
    return currentState;
}