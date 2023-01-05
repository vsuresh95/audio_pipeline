#ifndef AUDIOBASE_H
#define AUDIOBASE_H

#define N_TIME_MARKERS 10

#include <BFormat.hpp>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

typedef union
{
    float Flt;
    unsigned Int;
} print_float_t;

class AudioBase {
public:
    unsigned long long StartTime;
    unsigned long long EndTime;
    unsigned long long TotalTime[N_TIME_MARKERS];

    char *Name;

    unsigned RandFactor;

    AudioBase();

    // Helper function to start timer, end timer and
    // capture the difference in TotalTime[Index].
    void StartCounter();
    void EndCounter(unsigned Index);

    // Helper function to perform log2 using baremetal
    unsigned log2(unsigned product);

    // Helper function to generate pseudo-random numbers in baremetal
    audio_t myRand();

    // Helper function to write to an internal scratch register
    // of the Ariane core (mscratch). These writes can be used
    // to identify different phases of the application in waveform.
    void WriteScratchReg(unsigned value);
};

#endif // AUDIOBASE_H