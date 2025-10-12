#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>
#include <Ticker.h>
#include "effect/EffectManager.h"
#include "Payload.h"

#define LED_PIN 0
#define NUM_LEDS 200
#define BRIGHTNESS 10
#define LED_TYPE SK6812
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 30

class LedControl {
private:
    CRGB leds[NUM_LEDS];
    EffectManager& effectManager;
    Ticker eventTicker;
    int eventStep = 0;
    int eventTotalFlashes = 0;
    CRGB eventColor;

    void eventFlash();
    void showEventNotification(const CRGB& color, int flashes = 8, int delayMs = 300);

public:
    LedControl(EffectManager& manager);
    void initLEDs();
    void changeState(const Payload& payload);
    void setLEDColor(uint32_t color);
    void setLEDBrightness(int brightness);
    void setLEDEffect(String effect);

    void showSuccess();
    void showError();
};

#endif