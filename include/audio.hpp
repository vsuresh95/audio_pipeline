#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>

#include <AmbisonicBinauralizer.hpp>
#include <AmbisonicProcessor.hpp>
#include <AmbisonicZoomer.hpp>
#include <AudioBase.hpp>

const float ERR_TH = 0.05;

class ABAudio : public AudioBase {
public:
    void Configure();
    void loadSource();
    void processBlock();
    void PrintTimeInfo(unsigned factor);
   
    // Decoder associated with this audio.
    AmbisonicBinauralizer decoder;
    // Ambisonics rotator associated with this audio.
    AmbisonicProcessor rotator;
    // Ambisonics zoomer associated with this audio.
    AmbisonicZoomer zoomer;

    /// Temporary BFormat file to sum up ambisonics.
    CBFormat sumBF;

    // Instance of FFT-FIR-IFFT chain object. This object is
    // eventually passed to rotator and decoder to perform the FFI
    // chains in the respective Process() functions.
    FFIChain FFIChainInst;

    // Output result buffer that binauralizer will populate.
    audio_t **resultSample;
};

#endif // AUDIO_H