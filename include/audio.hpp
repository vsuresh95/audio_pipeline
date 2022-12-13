#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>

#include <AmbisonicBinauralizer.hpp>
#include <AmbisonicProcessor.hpp>
#include <AmbisonicZoomer.hpp>
#include <AudioBase.hpp>

class ABAudio : public AudioBase {
public:
    void Configure();
    void loadSource();
    void processBlock();
    void PrintTimeInfo(unsigned factor);
    
    // decoder associated with this audio
    AmbisonicBinauralizer decoder;
    // ambisonics rotator associated with this audio
    AmbisonicProcessor rotator;
    // ambisonics zoomer associated with this audio
    AmbisonicZoomer zoomer;

    /// Temporary BFormat file to sum up ambisonics
    CBFormat sumBF;

    FFIChain FFIChainInst;

    audio_t **resultSample;
};

#endif // AUDIO_H