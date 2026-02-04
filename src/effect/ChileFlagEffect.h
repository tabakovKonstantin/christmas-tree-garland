#ifndef CHILE_FLAG_EFFECT_H
#define CHILE_FLAG_EFFECT_H

#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class ChileFlagEffect : public Effect
{
private:
    Ticker effectTimer;
    uint16_t offset = 0;
    CRGBPalette16 palette;

    void internalRun(CRGB *leds, int numLeds)
    {
        for (int i = 0; i < numLeds; i++)
        {
            uint8_t index = (i * 8) - offset; 
            leds[i] = ColorFromPalette(palette, index, 255, LINEARBLEND);
        }
        
        offset++; 
        FastLED.show();
    }

public:
    ChileFlagEffect() {
        // Chile Flag colors: Blue, White, Red.
        // Palette: Blue, White, Red, Red to approximate visual proportions.
        palette = CRGBPalette16(CRGB::Blue, CRGB::White, CRGB::Red, CRGB::Red);
    }

    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(30, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};

#endif