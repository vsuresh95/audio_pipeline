#ifndef DECODER_H
#define DECODER_H

#include "kiss_fftr.hpp"

class AmbisonicBinauralizer {
public:
    unsigned m_nChannelCount;

    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    float m_fFFTScaler;
    unsigned m_nOverlapLength;

    struct kiss_fftr_state *m_pFFT_cfg;
    struct kiss_fftr_state *m_pIFFT_cfg;
    kiss_fft_cpx ***m_ppcpFilters;
    kiss_fft_cpx *m_pcpScratch;

    float *m_pfScratchBufferA;
    float *m_pfScratchBufferB;
    float *m_pfScratchBufferC;
    float **m_pfOverlap;

    void Configure(unsigned nSampleRate, unsigned nBlockSize, unsigned nChannels);
    
    void Process(CBFormat *pBFSrc, float **ppfDst);
};

#endif // DECODER_H