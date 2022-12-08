#ifndef ROTATOR_H
#define ROTATOR_H

#include "kiss_fftr.hpp"

class AmbisonicProcessor {
public:
    unsigned m_nChannelCount;
    unsigned m_nOrder;

    float* m_pfTempSample;

    kiss_fftr_cfg m_pFFT_psych_cfg;
    kiss_fftr_cfg m_pIFFT_psych_cfg;

    float* m_pfScratchBufferA;
    float** m_pfOverlap;
    unsigned m_nFFTSize;
    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nOverlapLength;
    unsigned m_nFFTBins;
    float m_fFFTScaler;

    kiss_fft_cpx** m_ppcpPsychFilters;
    kiss_fft_cpx* m_pcpScratch;

    float m_fCosAlpha;
    float m_fSinAlpha;
    float m_fCosBeta;
    float m_fSinBeta;
    float m_fCosGamma;
    float m_fSinGamma;
    float m_fCos2Alpha;
    float m_fSin2Alpha;
    float m_fCos2Beta;
    float m_fSin2Beta;
    float m_fCos2Gamma;
    float m_fSin2Gamma;
    float m_fCos3Alpha;
    float m_fSin3Alpha;
    float m_fCos3Beta;
    float m_fSin3Beta;
    float m_fCos3Gamma;
    float m_fSin3Gamma;

    void Configure(unsigned nBlockSize, unsigned nChannels);

    void updateRotation();

    void Process(CBFormat *pBFSrcDst, unsigned nSamples);

    void ProcessOrder1_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder2_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder3_3D(CBFormat* pBFSrcDst, unsigned nSamples);

    void ShelfFilterOrder(CBFormat* pBFSrcDst, unsigned nSamples);
};

#endif // ROTATOR_H