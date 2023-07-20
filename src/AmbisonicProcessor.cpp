#include <stdio.h>
#include <cstring>
#include <AmbisonicProcessor.hpp>
#include <RotateOrderOptimized.hpp>

#if (USE_REAL_DATA == 1)
#include <m_ppcpPsychFilters.hpp>
#include <RotationParams.hpp>
#endif // USE_REAL_DATA

void AmbisonicProcessor::Configure(unsigned nBlockSize, unsigned nChannels) {
    Name = (char *) "PSYCHOACOUSTIC FILTER";

    m_nOrder = NORDER;
    m_nChannelCount = nChannels;

    // Hard-coded based on observations from Linux app.
    unsigned nbTaps = 101;

    m_nBlockSize = nBlockSize;
    m_nTaps = nbTaps;

    // What will the overlap size be?
    m_nOverlapLength = m_nBlockSize < m_nTaps ? m_nBlockSize - 1 : m_nTaps - 1;

    // How large does the FFT need to be?
    m_nFFTSize = 1;
    while(m_nFFTSize < (m_nBlockSize + m_nTaps + m_nOverlapLength))
        m_nFFTSize <<= 1;

    // How many bins is that?
    m_nFFTBins = m_nFFTSize / 2 + 1;

    // What do we need to scale the result of the iFFT by?
    m_fFFTScaler = 1.f / m_nFFTSize;

    // Scratch buffer for rotate order - set to 0.
    m_pfTempSample = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));
    MyMemset(m_pfTempSample, 0, m_nChannelCount * sizeof(audio_t));

    // Allocate buffers for overlap operation.
    // In psycho-acoustic filter, the overlap operation stores overlap
    // information that carry forward from each audio block to the
    // next, for each channel.
    m_pfOverlap = (audio_t **) aligned_malloc(m_nChannelCount * sizeof(audio_t *));
    for(unsigned i = 0; i < m_nChannelCount; i++) {
        m_pfOverlap[i] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));
        MyMemset(m_pfOverlap[i], 0, m_nOverlapLength * sizeof(audio_t));
    }

    // Scratch buffer to hold the time domain samples - set to 0.
    m_pfScratchBufferA = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));
    MyMemset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(audio_t));

    // Scratch buffer to hold the frequency domain samples - set to 0.
    m_pcpScratch = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
    MyMemset(m_pcpScratch, 0, m_nFFTBins * sizeof(kiss_fft_cpx));

    // Allocate FFT and iFFT for new size - includes twiddle factor allocation.
    m_pFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    // Allocate space for filters for FIR. This is not different for all channels.
    // Every channel with same sqrt() value shares the filters.
    // i.e., NORDER+1 channels worth of filters.
    m_ppcpPsychFilters = (kiss_fft_cpx **) aligned_malloc((NORDER+1) * sizeof(kiss_fft_cpx *));
    for(unsigned i = 0; i < NORDER+1; i++) {
        m_ppcpPsychFilters[i] = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
        MyMemset(m_ppcpPsychFilters[i], 0, m_nFFTBins * sizeof(kiss_fft_cpx));
    }

    // Initialize Psychoacoustic filter values. We intitialize random data
    // instead of calculating them as in the Linux app.
#if (DO_DATA_INIT == 1)
    for(unsigned niChannel = 0; niChannel < NORDER+1; niChannel++) {
        for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
        #if (USE_REAL_DATA == 1)
            m_ppcpPsychFilters[niChannel][niSample].r = Orig_m_ppcpPsychFilters[niChannel][2*niSample];
            m_ppcpPsychFilters[niChannel][niSample].i = Orig_m_ppcpPsychFilters[niChannel][2*niSample+1];
        #else
            m_ppcpPsychFilters[niChannel][niSample].r = myRand();
            m_ppcpPsychFilters[niChannel][niSample].i = myRand();
        #endif
        }
    }
#endif
}

void AmbisonicProcessor::updateRotation() {
#if (USE_REAL_DATA == 1)
    m_fCosAlpha = Orig_RotationParams[0];
    m_fSinAlpha = Orig_RotationParams[1];
    m_fCosBeta = Orig_RotationParams[2];
    m_fSinBeta = Orig_RotationParams[3];
    m_fCosGamma = Orig_RotationParams[4];
    m_fSinGamma = Orig_RotationParams[5];
    m_fCos2Alpha = Orig_RotationParams[6];
    m_fSin2Alpha = Orig_RotationParams[7];
    m_fCos2Beta = Orig_RotationParams[8];
    m_fSin2Beta = Orig_RotationParams[9];
    m_fCos2Gamma = Orig_RotationParams[10];
    m_fSin2Gamma = Orig_RotationParams[11];
    m_fCos3Alpha = Orig_RotationParams[12];
    m_fSin3Alpha = Orig_RotationParams[13];
    m_fCos3Beta = Orig_RotationParams[14];
    m_fSin3Beta = Orig_RotationParams[15];
    m_fCos3Gamma = Orig_RotationParams[16];
    m_fSin3Gamma = Orig_RotationParams[17];
#else
    m_fCosAlpha = myRand();
    m_fSinAlpha = myRand();
    m_fCosBeta = myRand();
    m_fSinBeta = myRand();
    m_fCosGamma = myRand();
    m_fSinGamma = myRand();
    m_fCos2Alpha = myRand();
    m_fSin2Alpha = myRand();
    m_fCos2Beta = myRand();
    m_fSin2Beta = myRand();
    m_fCos2Gamma = myRand();
    m_fSin2Gamma = myRand();
    m_fCos3Alpha = myRand();
    m_fSin3Alpha = myRand();
    m_fCos3Beta = myRand();
    m_fSin3Beta = myRand();
    m_fCos3Gamma = myRand();
    m_fSin3Gamma = myRand();
#endif

	if (DO_ROTATE_OFFLOAD) {
        FFIChainInst.RotateInst.m_fCosAlpha = FLOAT_TO_FIXED_WRAP(m_fCosAlpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSinAlpha = FLOAT_TO_FIXED_WRAP(m_fSinAlpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCosBeta = FLOAT_TO_FIXED_WRAP(m_fCosBeta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSinBeta = FLOAT_TO_FIXED_WRAP(m_fSinBeta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCosGamma = FLOAT_TO_FIXED_WRAP(m_fCosGamma, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSinGamma = FLOAT_TO_FIXED_WRAP(m_fSinGamma, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos2Alpha = FLOAT_TO_FIXED_WRAP(m_fCos2Alpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin2Alpha = FLOAT_TO_FIXED_WRAP(m_fSin2Alpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos2Beta = FLOAT_TO_FIXED_WRAP(m_fCos2Beta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin2Beta = FLOAT_TO_FIXED_WRAP(m_fSin2Beta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos2Gamma = FLOAT_TO_FIXED_WRAP(m_fCos2Gamma, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin2Gamma = FLOAT_TO_FIXED_WRAP(m_fSin2Gamma, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos3Alpha = FLOAT_TO_FIXED_WRAP(m_fCos3Alpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin3Alpha = FLOAT_TO_FIXED_WRAP(m_fSin3Alpha, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos3Beta = FLOAT_TO_FIXED_WRAP(m_fCos3Beta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin3Beta = FLOAT_TO_FIXED_WRAP(m_fSin3Beta, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fCos3Gamma = FLOAT_TO_FIXED_WRAP(m_fCos3Gamma, ROTATE_FX_IL);
        FFIChainInst.RotateInst.m_fSin3Gamma = FLOAT_TO_FIXED_WRAP(m_fSin3Gamma, ROTATE_FX_IL);
    }
}

void AmbisonicProcessor::Process(CBFormat *pBFSrcDst, unsigned nSamples) {
    // if (DO_PP_CHAIN_OFFLOAD) {
    //     StartCounter();
    //     FFIChainInst.m_nOverlapLength = m_nOverlapLength;
    //     if (USE_AUDIO_DMA) {
    //         FFIChainInst.PsychoProcessDMA(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    //     } else {
    //         FFIChainInst.PsychoProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    //     }
    //     EndCounter(0);
    // } else if (DO_CHAIN_OFFLOAD) {
    //     StartCounter();
    //     FFIChainInst.m_nOverlapLength = m_nOverlapLength;
    //     FFIChainInst.PsychoRegularProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    //     EndCounter(0);
    // } else if (DO_NP_CHAIN_OFFLOAD) {
    //     StartCounter();
    //     FFIChainInst.m_nOverlapLength = m_nOverlapLength;
    //     FFIChainInst.PsychoNonPipelineProcess(pBFSrcDst, m_ppcpPsychFilters, m_pfOverlap);
    //     EndCounter(0);
    // } else {
    //     ShelfFilterOrder(pBFSrcDst, nSamples);
    // }

    StartCounter();
    if (DO_ROTATE_OFFLOAD) {        
        FFIChainInst.OffloadRotateOrder(pBFSrcDst);
    } else {
        WriteScratchReg(0x10);
        if(m_nOrder >= 1) {
            // ProcessOrder1_3D(pBFSrcDst, nSamples);
            ProcessOrder1_3D_Optimized(pBFSrcDst, nSamples);
        }
        WriteScratchReg(0);
        WriteScratchReg(0x20);
        if(m_nOrder >= 2) {
            ProcessOrder2_3D_Optimized(pBFSrcDst, nSamples);
        }
        WriteScratchReg(0);
        WriteScratchReg(0x40);
        if(m_nOrder >= 3) {
            ProcessOrder3_3D_Optimized(pBFSrcDst, nSamples);
        }
        WriteScratchReg(0);
    }
    EndCounter(3);
}

void AmbisonicProcessor::ProcessOrder1_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    /* Rotations are performed in the following order:
        1 - rotation around the z-axis
        2 - rotation around the *new* y-axis (y')
        3 - rotation around the new z-axis (z'')
    This is different to the rotations obtained from the video, which are around z, y' then x''.
    The rotation equations used here work for third order. However, for higher orders a recursive algorithm
    should be considered.*/
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosAlpha;
        m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
        m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinAlpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
        pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ] * m_fCosBeta
                            +  m_pfTempSample[kX] * m_fSinBeta;
        pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX] * m_fCosBeta
                            - m_pfTempSample[kZ] * m_fSinBeta;

        // Gamma rotation
        m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosGamma;
        m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
        m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinGamma;

        pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
        pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
        pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ];
    }
}

void AmbisonicProcessor::ProcessOrder2_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    audio_t fSqrt3 = sqrt(3.f);

    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Alpha
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Alpha;
        m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosAlpha;
        m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
        m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinAlpha;
        m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Alpha
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Alpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kV][niSample] = -m_fSinBeta * m_pfTempSample[kT]
                                        + m_fCosBeta * m_pfTempSample[kV];
        pBFSrcDst->m_ppfChannels[kT][niSample] = -m_fCosBeta * m_pfTempSample[kT]
                                        + m_fSinBeta * m_pfTempSample[kV];
        pBFSrcDst->m_ppfChannels[kR][niSample] = (0.75f * m_fCos2Beta + 0.25f) * m_pfTempSample[kR]
                            + (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * m_pfTempSample[kU]
                            + (fSqrt3 * m_fSinBeta * m_fCosBeta) * m_pfTempSample[kS];
        pBFSrcDst->m_ppfChannels[kS][niSample] = m_fCos2Beta * m_pfTempSample[kS]
                            - fSqrt3 * m_fCosBeta * m_fSinBeta * m_pfTempSample[kR]
                            + m_fCosBeta * m_fSinBeta * m_pfTempSample[kU];
        pBFSrcDst->m_ppfChannels[kU][niSample] = (0.25f * m_fCos2Beta + 0.75f) * m_pfTempSample[kU]
                            - m_fCosBeta * m_fSinBeta * m_pfTempSample[kS]
                            +0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * m_pfTempSample[kR];

        // Gamma rotation
        m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Gamma
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Gamma;
        m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosGamma;

        m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
        m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinGamma;
        m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Gamma
                            + pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Gamma;

        pBFSrcDst->m_ppfChannels[kR][niSample] = m_pfTempSample[kR];
        pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
        pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
        pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
        pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
    }
}

void AmbisonicProcessor::ProcessOrder3_3D(CBFormat* pBFSrcDst, unsigned nSamples)
{
    /* (should move these somewhere that does recompute each time) */
    audio_t fSqrt3_2 = sqrt(3.f/2.f);
    audio_t fSqrt15 = sqrt(15.f);
    audio_t fSqrt5_2 = sqrt(5.f/2.f);

    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        // Alpha rotation
        m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Alpha
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Alpha;
        m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Alpha
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Alpha;
        m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinAlpha
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosAlpha;
        m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
        m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosAlpha
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinAlpha;
        m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Alpha
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Alpha;
        m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Alpha
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Alpha;

        // Beta rotation
        pBFSrcDst->m_ppfChannels[kQ][niSample] = 0.125f * m_pfTempSample[kQ] * (5.f + 3.f*m_fCos2Beta)
                    - fSqrt3_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kM] * pow(m_fSinBeta,2.0f);
        pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO] * m_fCos2Beta
                    - fSqrt5_2 * m_pfTempSample[kM] * m_fCosBeta * m_fSinBeta
                    + fSqrt3_2 * m_pfTempSample[kQ] * m_fCosBeta * m_fSinBeta;
        pBFSrcDst->m_ppfChannels[kM][niSample] = 0.125f * m_pfTempSample[kM] * (3.f + 5.f*m_fCos2Beta)
                    - fSqrt5_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kQ] * pow(m_fSinBeta,2.0f);
        pBFSrcDst->m_ppfChannels[kK][niSample] = 0.25f * m_pfTempSample[kK] * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
                    + 0.5f * fSqrt15 * m_pfTempSample[kN] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.5f * fSqrt5_2 * m_pfTempSample[kP] * pow(m_fSinBeta,3.f)
                    + 0.125f * fSqrt3_2 * m_pfTempSample[kL] * (m_fSinBeta + 5.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kL][niSample] = 0.0625f * m_pfTempSample[kL] * (m_fCosBeta + 15.f * m_fCos3Beta)
                    + 0.25f * fSqrt5_2 * m_pfTempSample[kN] * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kP] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.125 * fSqrt3_2 * m_pfTempSample[kK] * (m_fSinBeta + 5.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kN][niSample] = 0.125f * m_pfTempSample[kN] * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                    + 0.25f * fSqrt3_2 * m_pfTempSample[kP] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.5f * fSqrt15 * m_pfTempSample[kK] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.125 * fSqrt5_2 * m_pfTempSample[kL] * (m_fSinBeta - 3.f * m_fSin3Beta);
        pBFSrcDst->m_ppfChannels[kP][niSample] = 0.0625f * m_pfTempSample[kP] * (15.f * m_fCosBeta + m_fCos3Beta)
                    - 0.25f * fSqrt3_2 * m_pfTempSample[kN] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * m_pfTempSample[kL] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.5 * fSqrt5_2 * m_pfTempSample[kK] * pow(m_fSinBeta,3.f);

        // Gamma rotation
        m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Gamma
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Gamma;
        m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Gamma
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Gamma;
        m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinGamma
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosGamma;
        m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
        m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosGamma
                            + pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinGamma;
        m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Gamma
                            + pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Gamma;
        m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Gamma
                            + pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Gamma;

        pBFSrcDst->m_ppfChannels[kQ][niSample] = m_pfTempSample[kQ];
        pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO];
        pBFSrcDst->m_ppfChannels[kM][niSample] = m_pfTempSample[kM];
        pBFSrcDst->m_ppfChannels[kK][niSample] = m_pfTempSample[kK];
        pBFSrcDst->m_ppfChannels[kL][niSample] = m_pfTempSample[kL];
        pBFSrcDst->m_ppfChannels[kN][niSample] = m_pfTempSample[kN];
        pBFSrcDst->m_ppfChannels[kP][niSample] = m_pfTempSample[kP];
    }
}

void AmbisonicProcessor::ShelfFilterOrder(CBFormat* pBFSrcDst, unsigned nSamples)
{
    kiss_fft_cpx cpTemp;

    unsigned iChannelOrder = 0;

    // Filter the Ambisonics channels
    // All  channels are filtered using linear phase FIR filters.
    // In the case of the 0th order signal (W channel) this takes the form of a delay
    // For all other channels shelf filters are used

    // Offload the entire task to be pipelined using shared memory.
    MyMemset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(audio_t));

    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        iChannelOrder = int(sqrt(niChannel));    //get the order of the current channel

        // Offload to regular invocation accelerators, or shared memory
        // invocation accelerators, or SW as per compiler flags.
        memcpy(m_pfScratchBufferA, pBFSrcDst->m_ppfChannels[niChannel], m_nBlockSize * sizeof(audio_t));
        MyMemset(&m_pfScratchBufferA[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(audio_t));

        StartCounter();
        kiss_fftr(m_pFFT_psych_cfg, m_pfScratchBufferA, m_pcpScratch);
        EndCounter(0);

        // Perform the convolution in the frequency domain
        StartCounter();
        for(unsigned ni = 0; ni < m_nFFTBins; ni++)
        {
            cpTemp.r = m_pcpScratch[ni].r * m_ppcpPsychFilters[iChannelOrder][ni].r
                        - m_pcpScratch[ni].i * m_ppcpPsychFilters[iChannelOrder][ni].i;
            cpTemp.i = m_pcpScratch[ni].r * m_ppcpPsychFilters[iChannelOrder][ni].i
                        + m_pcpScratch[ni].i * m_ppcpPsychFilters[iChannelOrder][ni].r;
            m_pcpScratch[ni] = cpTemp;
        }
        EndCounter(1);

        // Convert from frequency domain back to time domain
        StartCounter();
        kiss_fftri(m_pIFFT_psych_cfg, m_pcpScratch, m_pfScratchBufferA);
        EndCounter(2);

        for(unsigned ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;

        memcpy(pBFSrcDst->m_ppfChannels[niChannel], m_pfScratchBufferA, m_nBlockSize * sizeof(audio_t));

        for(unsigned ni = 0; ni < m_nOverlapLength; ni++) {
            pBFSrcDst->m_ppfChannels[niChannel][ni] += m_pfOverlap[niChannel][ni];
        }

        memcpy(m_pfOverlap[niChannel], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(audio_t));
    }
    
}

void AmbisonicProcessor::PrintTimeInfo(unsigned factor) {
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        printf("Psycho Chain Total\t = %llu\n", TotalTime[0]/factor);
    } else {
        printf("Psycho FFT\t = %llu\n", TotalTime[0]/factor);
        printf("Psycho FIR\t = %llu\n", TotalTime[1]/factor);
        printf("Psycho IFFT\t = %llu\n", TotalTime[2]/factor);
    }
    printf("Rotate Order\t = %llu\n", TotalTime[3]/factor);

    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        FFIChainInst.PrintTimeInfo(factor, true);
    }
}