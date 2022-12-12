#include <stdio.h>
#include <cstring>
#include <AmbisonicProcessor.hpp>

void AmbisonicProcessor::Configure(unsigned nBlockSize, unsigned nChannels) {
    Name = (char *) "PSYCHOACOUSTIC FILTER";

    m_nOrder = NORDER;
    m_nChannelCount = nChannels;

    m_pfTempSample = (audio_t *) aligned_malloc(m_nChannelCount * sizeof(audio_t));
    memset(m_pfTempSample, 0, m_nChannelCount * sizeof(audio_t));

    // All optimisation filters have the same number of taps so take from the first order 3D impulse response arbitrarily
    unsigned nbTaps = 101;

    m_nBlockSize = nBlockSize;
    m_nTaps = nbTaps;

    //What will the overlap size be?
    m_nOverlapLength = m_nBlockSize < m_nTaps ? m_nBlockSize - 1 : m_nTaps - 1;

    //How large does the FFT need to be
    m_nFFTSize = 1;
    while(m_nFFTSize < (m_nBlockSize + m_nTaps + m_nOverlapLength))
        m_nFFTSize <<= 1;

    //How many bins is that
    m_nFFTBins = m_nFFTSize / 2 + 1;

    //What do we need to scale the result of the iFFT by
    m_fFFTScaler = 1.f / m_nFFTSize;

    //Allocate buffers
    m_pfOverlap = (audio_t **) aligned_malloc(m_nChannelCount * sizeof(audio_t *));
    for(unsigned i = 0; i < m_nChannelCount; i++) {
        m_pfOverlap[i] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));
    }

    m_pfScratchBufferA = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));

    m_ppcpPsychFilters = (kiss_fft_cpx **) aligned_malloc(NORDER * sizeof(kiss_fft_cpx *));
    for(unsigned i = 0; i < NORDER; i++) {
        m_ppcpPsychFilters[i] = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
    }

    m_pcpScratch = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));

    //Allocate FFT and iFFT for new size
    m_pFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    printf("[%s] Initializing psycho filters\n", Name);

    for(unsigned niChannel = 0; niChannel < NORDER; niChannel++) {
        for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
            m_ppcpPsychFilters[niChannel][niSample].r = rand() % 100;
            m_ppcpPsychFilters[niChannel][niSample].i = rand() % 100;
        }
    }
}

void AmbisonicProcessor::updateRotation() {
    m_fCosAlpha = rand() % 100;
    m_fSinAlpha = rand() % 100;
    m_fCosBeta = rand() % 100;
    m_fSinBeta = rand() % 100;
    m_fCosGamma = rand() % 100;
    m_fSinGamma = rand() % 100;
    m_fCos2Alpha = rand() % 100;
    m_fSin2Alpha = rand() % 100;
    m_fCos2Beta = rand() % 100;
    m_fSin2Beta = rand() % 100;
    m_fCos2Gamma = rand() % 100;
    m_fSin2Gamma = rand() % 100;
    m_fCos3Alpha = rand() % 100;
    m_fSin3Alpha = rand() % 100;
    m_fCos3Beta = rand() % 100;
    m_fSin3Beta = rand() % 100;
    m_fCos3Gamma = rand() % 100;
    m_fSin3Gamma = rand() % 100;
}

void AmbisonicProcessor::Process(CBFormat *pBFSrcDst, unsigned nSamples) {
    ShelfFilterOrder(pBFSrcDst, nSamples);
    if(m_nOrder >= 1) {
        StartCounter();
        ProcessOrder1_3D(pBFSrcDst, nSamples);
        EndCounter(3);
    }
    if(m_nOrder >= 2) {
        StartCounter();
        ProcessOrder2_3D(pBFSrcDst, nSamples);
        EndCounter(4);
    }
    if(m_nOrder >= 3) {
        StartCounter();
        ProcessOrder3_3D(pBFSrcDst, nSamples);
        EndCounter(5);
    }
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
    memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(audio_t));

    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        iChannelOrder = int(sqrt(niChannel));    //get the order of the current channel

        if (DO_NP_CHAIN_OFFLOAD) {
            StartCounter();
            FFIChainInst.NonPipelineProcess(pBFSrcDst, m_ppcpPsychFilters[iChannelOrder], niChannel);
            EndCounter(0);
        } else {
            memcpy(m_pfScratchBufferA, pBFSrcDst->m_ppfChannels[niChannel], m_nBlockSize * sizeof(audio_t));
            memset(&m_pfScratchBufferA[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(audio_t));

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
        }

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
    printf("---------------------------------------------\n");
    printf("TOTAL TIME FROM %s\n", Name);
    printf("---------------------------------------------\n");
    if (DO_NP_CHAIN_OFFLOAD) {
        printf("Psycho Chain\t = %lu\n", TotalTime[0]/factor);
    } else {
        printf("Psycho FFT\t = %lu\n", TotalTime[0]/factor);
        printf("Psycho FIR\t = %lu\n", TotalTime[1]/factor);
        printf("Psycho IFFT\t = %lu\n", TotalTime[2]/factor);
    }
    printf("Rotate O1\t = %lu\n", TotalTime[3]/factor);
    printf("Rotate O2\t = %lu\n", TotalTime[4]/factor);
    printf("Rotate O3\t = %lu\n", TotalTime[5]/factor);
    printf("\n");
}