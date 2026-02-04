#ifndef EFFECT_FACTORY_H
#define EFFECT_FACTORY_H

#include "Effect.h"
#include "RainbowEffect.h"
#include "SparkleEffect.h"
#include "SmoothWaveEffect.h"
#include "StrangerThingsEffect.h"
#include "FireEffect.h"
#include "TurkeyFlagEffect.h"
#include "RussiaFlagEffect.h"
#include "ChileFlagEffect.h"

class EffectFactory
{
public:
    static Effect *createEffect(const String &effectName)
    {
        if (effectName == "Rainbow")
        {
            return new RainbowEffect();
        }

        if (effectName == "Smooth wave")
        {
            return new SmoothWaveEffect();
        }

        if (effectName == "Sparkle")
        {
            return new SparkleEffect();
        }

        if (effectName == "Stranger Things")
        {
            return new StrangerThingsEffect();
        }

        if (effectName == "Halloween Flame")
        {
            return new FireEffect();
        }

        if (effectName == "Turkey Flag")
        {
            return new TurkeyFlagEffect();
        }

        if (effectName == "Russia Flag")
        {
            return new RussiaFlagEffect();
        }

        if (effectName == "Chile Flag")
        {
            return new ChileFlagEffect();
        }

        return nullptr;
    }
};

#endif