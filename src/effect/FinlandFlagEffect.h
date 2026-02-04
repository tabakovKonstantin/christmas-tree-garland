#ifndef FINLAND_FLAG_EFFECT_H
#define FINLAND_FLAG_EFFECT_H

#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class FinlandFlagEffect : public Effect
{
private:
    Ticker effectTimer;
    uint16_t offset = 0;

    void internalRun(CRGB *leds, int numLeds)
    {
        // Smooth wave blending between White and Blue (Finnish flag colors)
        for (int i = 0; i < numLeds; i++)
        {
            // Create a sine wave value [0..255] based on position and time
            uint8_t ratio = sin8(i * 8 + offset);
            
            // Blend White and Blue based on the ratio
            // This creates a smooth "waving flag" gradient look
            leds[i] = blend(CRGB::White, CRGB::Blue, ratio);
        }
        
        offset += 2; // Animate the wave
        FastLED.show();
    }

public:
    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(30, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};

#endif