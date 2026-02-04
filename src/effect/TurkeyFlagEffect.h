#ifndef TURKEY_FLAG_EFFECT_H
#define TURKEY_FLAG_EFFECT_H

#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class TurkeyFlagEffect : public Effect
{
private:
    Ticker effectTimer;
    uint16_t offset = 0;
    CRGBPalette16 palette;

    void internalRun(CRGB *leds, int numLeds)
    {
        for (int i = 0; i < numLeds; i++)
        {
            // Map pixel index to palette index
            // Scaled by 8 to create visible wave segments
            uint8_t index = (i * 8) + offset; 
            leds[i] = ColorFromPalette(palette, index, 255, LINEARBLEND);
        }
        
        offset += 2; 
        FastLED.show();
    }

public:
    TurkeyFlagEffect() {
        // Turkey Flag: Mostly Red with White features.
        // Palette anchors: Red, Red, White, Red -> Creates a Red wave with White pulse.
        palette = CRGBPalette16(CRGB::Red, CRGB::Red, CRGB::White, CRGB::Red);
    }

    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(30, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};

#endif