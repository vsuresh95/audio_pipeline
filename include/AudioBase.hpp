#ifndef AUDIOBASE_H
#define AUDIOBASE_H

#define N_TIME_MARKERS 10

#include <BFormat.hpp>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

class AudioBase {
public:
    unsigned long long StartTime;
    unsigned long long EndTime;
    unsigned long long TotalTime[N_TIME_MARKERS];

    char *Name;

    AudioBase();

    void StartCounter();
    void EndCounter(unsigned Index);
    unsigned log2(unsigned product);
    audio_t myRand();
    void WriteScratchReg(unsigned value);

    unsigned RandFactor;
};

#endif // AUDIOBASE_H