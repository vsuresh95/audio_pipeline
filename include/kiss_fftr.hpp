#ifndef KISS_FFTR_H
#define KISS_FFTR_H

#include "kiss_fft.hpp"

typedef struct kiss_fftr_state *kiss_fftr_cfg;

kiss_fftr_cfg kiss_fftr_alloc(int nfft, int inverse_fft, void *mem, unsigned *lenmem);

void kiss_fftr(kiss_fftr_cfg cfg, const kiss_fft_scalar *timedata, kiss_fft_cpx *freqdata);

void kiss_fftri(kiss_fftr_cfg cfg, const kiss_fft_cpx *freqdata, kiss_fft_scalar *timedata);

#endif // KISS_FFTR_H