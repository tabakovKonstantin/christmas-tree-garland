#include "RainbowEffect.h"

uint8_t hueStart = 0;      // Начальный оттенок
uint8_t deltaHue = 10;     // Разница оттенков между соседними светодиодами
uint16_t speedDelay = 50;  // Задержка между шагами (в мс)
uint8_t currentOffset = 0; // Смещение для создания эффекта движения

void RainbowEffect::run(CRGB *leds, int numLeds)
{
    effectTimer.attach_ms(20, [this, leds, numLeds]()
                          { internalRun(leds, numLeds); });
}

void RainbowEffect::internalRun(CRGB *leds, int numLeds)
{
    for (int i = 0; i < numLeds; i++)
    {
        leds[i] = CHSV((hueStart + currentOffset + i * deltaHue) % 255, 255, 255);
    }

    currentOffset++;
    // Serial.println(currentOffset); // Removed debug output
}