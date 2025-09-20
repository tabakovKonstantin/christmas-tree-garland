#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class TreeEffect : public Effect
{
private:
    Ticker effectTimer;
    unsigned long lastUpdateTime = 0;
    int sparkleDelay = 80;  // Задержка для обновления эффекта
    int sparkleChance = 50; // Вероятность включения случайного светодиода
    int numSparkles = 10;

    void internalRun(CRGB *leds, int numLeds)
    {
        for (int i = 0; i < numLeds; i++)
        {
            if (i < 38 || i > 91 )
            {
                leds[i] = CRGB::White; // Первые 28 светодиодов белые
            }
            else if ((i >= 38 && i <= 41) || i == 91)
            {
                leds[i] = CRGB::DarkOrange; // Светодиоды с 40 по 45 коричневые
            }
            
            else
            {
                if (random(0, 100) < 10) // 30% вероятность мерцания
                {
                    leds[i] = CRGB::RoyalBlue; // Мерцающий светодиод жёлтым
                }
                else
                {
                    leds[i] = CRGB::Green; // Остальные зелёные
                }
            }
        }

        FastLED.show();
    }

public:
    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(100, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};
