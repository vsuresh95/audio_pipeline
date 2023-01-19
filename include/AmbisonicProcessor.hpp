#ifndef ROTATOR_H
#define ROTATOR_H

#include <kiss_fftr.hpp>
#include <AudioBase.hpp>
#include <FFIChain.hpp>

class AmbisonicProcessor : public AudioBase {
public:
    unsigned m_nChannelCount;
    unsigned m_nOrder;

    audio_t* m_pfTempSample;

    kiss_fftr_cfg m_pFFT_psych_cfg;
    kiss_fftr_cfg m_pIFFT_psych_cfg;

    audio_t* m_pfScratchBufferA;
    audio_t** m_pfOverlap;
    unsigned m_nFFTSize;
    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nOverlapLength;
    unsigned m_nFFTBins;
    audio_t m_fFFTScaler;

    kiss_fft_cpx** m_ppcpPsychFilters;
    kiss_fft_cpx* m_pcpScratch;

    audio_t m_fCosAlpha;
    audio_t m_fSinAlpha;
    audio_t m_fCosBeta;
    audio_t m_fSinBeta;
    audio_t m_fCosGamma;
    audio_t m_fSinGamma;
    audio_t m_fCos2Alpha;
    audio_t m_fSin2Alpha;
    audio_t m_fCos2Beta;
    audio_t m_fSin2Beta;
    audio_t m_fCos2Gamma;
    audio_t m_fSin2Gamma;
    audio_t m_fCos3Alpha;
    audio_t m_fSin3Alpha;
    audio_t m_fCos3Beta;
    audio_t m_fSin3Beta;
    audio_t m_fCos3Gamma;
    audio_t m_fSin3Gamma;

    void Configure(unsigned nBlockSize, unsigned nChannels);

    void updateRotation();

    void Process(CBFormat *pBFSrcDst, unsigned nSamples);

    void ProcessOrder1_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder2_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder3_3D(CBFormat* pBFSrcDst, unsigned nSamples);

    void ProcessOrder1_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder2_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder3_3D_Optimized(CBFormat* pBFSrcDst, unsigned nSamples);

    void ShelfFilterOrder(CBFormat* pBFSrcDst, unsigned nSamples);

    void PrintTimeInfo(unsigned factor);

    FFIChain FFIChainInst;
};

#endif // ROTATOR_H