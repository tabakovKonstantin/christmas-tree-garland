#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

// Эффект имитации горящего пламени, мягко переливающегося как свеча.
// Подходит для включения на Хэллоуин.
class FireEffect : public Effect
{
private:
    Ticker effectTimer;
    unsigned long lastUpdateTime = 0;
    int cooling = 55;      // скорость охлаждения
    int sparking = 120;    // частота искр
    int speedDelay = 30;   // задержка между обновлениями

    void internalRun(CRGB *leds, int numLeds)
    {
        static byte heat[200]; // тепловая карта, максимум 200 светодиодов

        unsigned long currentMillis = millis();
        if (currentMillis - lastUpdateTime < speedDelay)
            return;
        lastUpdateTime = currentMillis;

        // 1. Охлаждение каждого элемента
        for (int i = 0; i < numLeds; i++)
        {
            heat[i] = qsub8(heat[i], random8(0, ((cooling * 10) / numLeds) + 2));
        }

        // 2. Перетекание тепла вверх
        for (int k = numLeds - 1; k >= 2; k--)
        {
            heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
        }

        // 3. Случайные "искра" внизу
        if (random8() < sparking)
        {
            int y = random8(7);
            heat[y] = qadd8(heat[y], random8(160, 255));
        }

        // 4. Преобразование тепла в цвет (палитра огня)
        for (int j = 0; j < numLeds; j++)
        {
            CRGB color = HeatColor(heat[j]);
            leds[j] = color;
        }

        FastLED.show();
    }

public:
    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(20, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};