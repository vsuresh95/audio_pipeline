#ifndef DECODER_H
#define DECODER_H

#include <kiss_fftr.hpp>
#include <AudioBase.hpp>
#include <FFIChain.hpp>

class AmbisonicBinauralizer : public AudioBase {
public:
    unsigned m_nChannelCount;

    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    audio_t m_fFFTScaler;
    unsigned m_nOverlapLength;

    struct kiss_fftr_state *m_pFFT_cfg;
    struct kiss_fftr_state *m_pIFFT_cfg;
    kiss_fft_cpx ***m_ppcpFilters;
    kiss_fft_cpx *m_pcpScratch;

    audio_t *m_pfScratchBufferA;
    audio_t *m_pfScratchBufferB;
    audio_t **m_pfOverlap;

    void Configure(unsigned nSampleRate, unsigned nBlockSize, unsigned nChannels);
    
    void Process(CBFormat *pBFSrc, audio_t **ppfDst);

    void PrintTimeInfo(unsigned factor);

    FFIChain FFIChainInst;
};

#endif // DECODER_H