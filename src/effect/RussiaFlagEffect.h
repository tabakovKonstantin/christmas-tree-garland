#ifndef RUSSIA_FLAG_EFFECT_H
#define RUSSIA_FLAG_EFFECT_H

#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class RussiaFlagEffect : public Effect
{
private:
    Ticker effectTimer;
    uint16_t offset = 0;
    CRGBPalette16 palette;

    void internalRun(CRGB *leds, int numLeds)
    {
        for (int i = 0; i < numLeds; i++)
        {
            // Tricolor wave
            uint8_t index = (i * 8) - offset; 
            leds[i] = ColorFromPalette(palette, index, 255, LINEARBLEND);
        }
        
        offset++;
        FastLED.show();
    }

public:
    RussiaFlagEffect() {
        // Russia Flag: White, Blue, Red.
        // Loop back to White for smooth transition.
        palette = CRGBPalette16(CRGB::White, CRGB::Blue, CRGB::Red, CRGB::White);
    }

    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(30, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};

#endif