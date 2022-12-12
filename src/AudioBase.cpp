#include <AudioBase.hpp>
#include <stdio.h>

AudioBase::AudioBase() {
    StartTime = 0;
    EndTime = 0;

	for (unsigned i = 0; i < N_TIME_MARKERS; i++)
    	TotalTime[i] = 0;

    Name = (char *) aligned_malloc(64 * sizeof(char *));
}

void AudioBase::StartCounter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (StartTime)
		:
		: "t0"
	);
}

void AudioBase::EndCounter(unsigned Index) {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (EndTime)
		:
		: "t0"
	);

    TotalTime[Index] += EndTime - StartTime;
}

unsigned AudioBase::log2(unsigned product) {
	unsigned exp = 0;

	while (product > 1) {
		exp++;
		product /= 2;
	}

	return exp;
}