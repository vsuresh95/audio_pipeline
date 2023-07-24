#ifndef KISS_FFTR_H
#define KISS_FFTR_H

#include "kiss_fft.hpp"

typedef struct kiss_fftr_state *kiss_fftr_cfg;

struct kiss_fftr_state {
    kiss_fft_cfg substate;
    kiss_fft_cpx * tmpbuf;
    kiss_fft_cpx * super_twiddles;
};

kiss_fftr_cfg kiss_fftr_alloc(int nfft, int inverse_fft, void *mem, unsigned *lenmem);

unsigned long long  kiss_fftr(kiss_fftr_cfg cfg,const kiss_fft_scalar *timedata_input,kiss_fft_cpx *freqdata);
/*
 input timedata has nfft scalar points
 output freqdata has nfft/2+1 complex points
*/

unsigned long long  kiss_fftri(kiss_fftr_cfg cfg,const kiss_fft_cpx *freqdata,kiss_fft_scalar *timedata_output);
/*
 input freqdata has  nfft/2+1 complex points
 output timedata has nfft scalar points
*/

unsigned long long KissGetCounter();
/*
 Timing helper functions and variables
*/

#endif // KISS_FFTR_H