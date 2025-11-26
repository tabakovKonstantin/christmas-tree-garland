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
#define UPDATES_PER_SECOND 120

class LedControl {
private:
    CRGB leds[NUM_LEDS];
    EffectManager& effectManager;
    Ticker eventTicker;
    int eventStep = 0;
    int eventTotalFlashes = 0;
    CRGB eventColor;

    bool isOn = false;

    // Текущее известное состояние лампы, которое
    // является single source of truth для MQTT state.
    Payload currentState;

    void eventFlash();
    void showEventNotification(const CRGB& color, int flashes = 8, int delayMs = 300);

    bool shouldUseWarmDefault(const Payload& payload,
                              bool hasBrightness,
                              bool hasColorRGB,
                              bool hasEffect,
                              bool isCurrentlyOn) const;
    void applyWarmDefault(int brightness);

public:
    LedControl(EffectManager& manager);
    void initLEDs();

    // Принимает инкрементальное состояние (как из MQTT),
    // мержит его с currentState, применяет к железу
    // и обновляет currentState.
    void changeState(const Payload& payload);

    void setLEDColor(uint32_t color);
    void setLEDBrightness(int brightness);
    void setLEDEffect(String effect);

    // Возвращаем текущий стейт для публикации в MQTT.
    const Payload& getCurrentState() const;

    void showSuccess();
    void showError();
};

#endif