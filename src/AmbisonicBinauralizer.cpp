#include <stdio.h>
#include <cstring>
#include <BFormat.hpp>
#include <AmbisonicBinauralizer.hpp>

extern "C" {
#include <esp_accelerator.h>
#include <esp_probe.h>
}

void AmbisonicBinauralizer::Configure(unsigned nSampleRate, unsigned nBlockSize, unsigned nChannels) {
    m_nChannelCount = nChannels;

    m_nTaps = 140;
    
    m_nBlockSize = nBlockSize;

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

    //Allocate scratch buffers
    m_pfScratchBufferA = (float *) aligned_malloc(m_nFFTSize * sizeof(float));
    m_pfScratchBufferB = (float *) aligned_malloc(m_nFFTSize * sizeof(float));
    m_pfScratchBufferC = (float *) aligned_malloc(m_nFFTSize * sizeof(float));

    //Allocate overlap-add buffers
    m_pfOverlap = (float **) aligned_malloc(2 * sizeof(float *));
    m_pfOverlap[0] = (float *) aligned_malloc(m_nOverlapLength * sizeof(float));
    m_pfOverlap[1] = (float *) aligned_malloc(m_nOverlapLength * sizeof(float));

    //Allocate FFT and iFFT for new size
    m_pFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    //Allocate the FFTBins for each channel, for each ear
    m_ppcpFilters = (kiss_fft_cpx ***) aligned_malloc(2 * sizeof(kiss_fft_cpx **));
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        m_ppcpFilters[niEar] = (kiss_fft_cpx **) aligned_malloc(m_nChannelCount * sizeof(kiss_fft_cpx *));
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            m_ppcpFilters[niEar][niChannel] = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
        }
    }

    m_pcpScratch = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
}

void AmbisonicBinauralizer::Process(CBFormat *pBFSrc, float **ppfDst) {
    unsigned niEar = 0;
    unsigned niChannel = 0;
    unsigned ni = 0;
    kiss_fft_cpx cpTemp;

    // Perform the convolution on both ears. Potentially more realistic results but requires double the number of
    // convolutions.
    for(niEar = 0; niEar < 2; niEar++)
    {
        memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(float));
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            memcpy(m_pfScratchBufferB, pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(float));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(float));

            kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);

            for(ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                            - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                            + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }

            kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);

            for(ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
        }
        for(ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
        memcpy(ppfDst[niEar], m_pfScratchBufferA, m_nBlockSize * sizeof(float));
        for(ni = 0; ni < m_nOverlapLength; ni++)
            ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
        memcpy(m_pfOverlap[niEar], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(float));
    }
}