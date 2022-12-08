#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

#include <BFormat.hpp>
#include <AmbisonicBinauralizer.hpp>
#include <AmbisonicProcessor.hpp>
#include <AmbisonicZoomer.hpp>

class ABAudio {
public:
    void Configure();
    void loadSource();
    void processBlock();
    
    // decoder associated with this audio
    AmbisonicBinauralizer decoder;
    // ambisonics rotator associated with this audio
    AmbisonicProcessor rotator;
    // ambisonics zoomer associated with this audio
    AmbisonicZoomer zoomer;
};

#endif // AUDIO_H