#include "LedControl.h"
#include "effect/EffectFactory.h"

LedControl::LedControl(EffectManager &manager) : effectManager(manager) {}

void LedControl::initLEDs()
{
    Serial.println("Initializing LEDs...");
    delay(1000);
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.delay(UPDATES_PER_SECOND);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear(true);
    FastLED.show();
}

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
    {
        fill_solid(leds, NUM_LEDS, eventColor);
    }
    else
    {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    }

    FastLED.show();
    eventStep++;
}

void LedControl::showEventNotification(const CRGB& color, int flashes, int delayMs)
{
    eventTicker.detach();
    eventStep = 0;
    eventTotalFlashes = flashes;
    eventColor = color;
    eventTicker.attach_ms(delayMs, [this]() { this->eventFlash(); });
}

void LedControl::showSuccess()
{
    showEventNotification(CRGB::Green, 3, 300);
}

void LedControl::showError()
{
    showEventNotification(CRGB::Red, 6, 600);
}

void LedControl::changeState(const Payload &payload)
{
    Serial.println("Applying new state...");

    bool hasBrightness = payload.brightness != -1;
    bool hasColorRGB = (payload.color.r != -1 && payload.color.g != -1 && payload.color.b != -1);
    bool hasEffect = (payload.effect != "null" && payload.effect.length() > 0);
    bool hasState = (payload.state.length() > 0);

    // Case 1: only brightness provided (no state/color/effect).
    if (hasBrightness && !hasColorRGB && !hasEffect && !hasState)
    {
        Serial.println("Only brightness provided, applying warm yellow default color.");
        setLEDBrightness(payload.brightness);

        CRGB warmYellow = CRGB(255, 200, 120);
        fill_solid(leds, NUM_LEDS, warmYellow);
        FastLED.show();

        Serial.println("Applying has finished");
        return;
    }

    // Case 2: state is ON + brightness, but no RGB and no effect
    // Example: {"state":"ON","brightness":8}
    if (hasBrightness && !hasColorRGB && !hasEffect && payload.state.equalsIgnoreCase("ON"))
    {
        Serial.println("State ON with brightness only, applying warm yellow default color.");
        setLEDBrightness(payload.brightness);

        CRGB warmYellow = CRGB(255, 200, 120);
        fill_solid(leds, NUM_LEDS, warmYellow);
        FastLED.show();

        Serial.println("Applying has finished");
        return;
    }

    if (hasBrightness)
    {
        setLEDBrightness(payload.brightness);
    }

    if (hasColorRGB)
    {
        uint32_t color = (payload.color.r << 16) | (payload.color.g << 8) | payload.color.b;
        setLEDColor(color);
    }

    if (payload.effect != "null")
    {
        setLEDEffect(payload.effect);
    }
    else if (payload.brightness == -1)
    {
        effectManager.setEffect(nullptr);
    }

    if (payload.state == "ON" && payload.color.r == -1 && payload.brightness == -1 && payload.effect == "null")
    {
        Serial.println("Turn on");
        fill_solid(leds, NUM_LEDS, CRGB::White);
    }

    if (payload.state == "OFF")
    {
        Serial.println("Turn off");
        FastLED.clear();
    }
    Serial.println("Applying has finished");
}

void LedControl::setLEDColor(uint32_t color)
{
    Serial.print("Set color: ");
    Serial.println(color);
    fill_solid(leds, NUM_LEDS, CRGB(color));
}

void LedControl::setLEDBrightness(int brightness)
{
    Serial.print("Set brightness: ");
    Serial.println(brightness);
    FastLED.setBrightness(brightness);
}

void LedControl::setLEDEffect(String effect)
{
    Serial.print("Set effect: ");
    Serial.println(effect);
    Effect *newEffect = EffectFactory::createEffect(effect);
    effectManager.setEffect(newEffect);
    effectManager.runEffect(leds, NUM_LEDS);
}