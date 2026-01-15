#ifndef STRANGER_THINGS_EFFECT_H
#define STRANGER_THINGS_EFFECT_H

#include <FastLED.h>
#include "Effect.h"
#include <Ticker.h>

class StrangerThingsEffect : public Effect
{
private:
    Ticker effectTimer;
    // Retro C9 bulb colors (Red, Green, Blue, Yellow, White)
    const uint32_t PALETTE[5] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0xFFFFFF};
    
    bool chaosMode = false;
    int chaosCounter = 0;

    void internalRun(CRGB *leds, int numLeds)
    {
        // 1. Random chance to enter Chaos Mode (Demogorgon is near)
        // Approx 0.5% chance per frame (~once every few seconds)
        if (!chaosMode && random8() < 2) { 
            chaosMode = true;
            chaosCounter = random8(20, 60); // Duration frames
        }

        if (chaosMode) {
            // Chaos: Flash everything randomly / nervous flickering
            fill_solid(leds, numLeds, CRGB::Black);
            
            // Random sparks
            for(int i=0; i < numLeds / 4; i++) {
                int pos = random16(numLeds);
                if(random8() > 128) {
                     leds[pos] = CRGB::White; // Intense white flashes
                } else {
                     leds[pos] = CRGB::Red;   // Scary red
                }
            }
            
            chaosCounter--;
            if (chaosCounter <= 0) {
                chaosMode = false;
                fill_solid(leds, numLeds, CRGB::Black); // Clear after chaos
            }
        } else {
            // "Communication" Mode: Slow, deliberate, retro colors
            
            // Fade existing lights
            for(int i=0; i<numLeds; i++) {
                leds[i].fadeToBlackBy(5);
            }

            // Light up a new one occasionally
            if (random8() < 40) { 
                int pos = random16(numLeds);
                // Only light up if it's currently dark
                if (leds[pos].getAverageLight() < 10) { 
                    uint32_t c = PALETTE[random8(5)];
                    leds[pos] = CRGB(c);
                }
            }
            
            // Nervous flicker for currently active lights
            for(int i=0; i<numLeds; i++) {
                if (leds[i].getAverageLight() > 10) {
                    // Randomly brighten or dim slightly to simulate old filament or voltage drop
                    uint8_t flicker = random8();
                    if(flicker > 200) {
                        leds[i] += CRGB(30,30,30);
                    } else if (flicker < 50) {
                        leds[i] -= CRGB(30,30,30);
                    }
                }
            }
        }
        FastLED.show();
    }

public:
    void run(CRGB *leds, int numLeds) override
    {
        effectTimer.attach_ms(40, [this, leds, numLeds]()
                              { internalRun(leds, numLeds); });
    }
};

#endif