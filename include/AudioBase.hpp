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
};

#endif // AUDIOBASE_H