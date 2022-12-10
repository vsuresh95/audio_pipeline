#include <stdio.h>
#include <cstring>
#include <AmbisonicBinauralizer.hpp>

void AmbisonicBinauralizer::Configure(unsigned nSampleRate, unsigned nBlockSize, unsigned nChannels) {
    Name = (char *) "BINAURALIZER";

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
    m_pfScratchBufferA = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));
    m_pfScratchBufferB = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));
    m_pfScratchBufferC = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));

    //Allocate overlap-add buffers
    m_pfOverlap = (audio_t **) aligned_malloc(2 * sizeof(audio_t *));
    m_pfOverlap[0] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));
    m_pfOverlap[1] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));

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

    printf("[%s] Initializing binaur filters\n", Name);

    for(unsigned niEar = 0; niEar < 2; niEar++) {
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
                m_ppcpFilters[niEar][niChannel][niSample].r = rand() % 100;
                m_ppcpFilters[niEar][niChannel][niSample].i = rand() % 100;
            }
        }
    }
}

void AmbisonicBinauralizer::Process(CBFormat *pBFSrc, audio_t **ppfDst) {
    unsigned niEar = 0;
    unsigned niChannel = 0;
    unsigned ni = 0;
    kiss_fft_cpx cpTemp;

    // Perform the convolution on both ears. Potentially more realistic results but requires double the number of
    // convolutions.
    for(niEar = 0; niEar < 2; niEar++)
    {
        memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(audio_t));
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            memcpy(m_pfScratchBufferB, pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(audio_t));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(audio_t));

            StartCounter();
            kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);
            EndCounter(0);

            StartCounter();
            for(ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                            - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                            + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            EndCounter(1);

            StartCounter();
            kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);
            EndCounter(2);

            for(ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
        }
        for(ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
        memcpy(ppfDst[niEar], m_pfScratchBufferA, m_nBlockSize * sizeof(audio_t));
        for(ni = 0; ni < m_nOverlapLength; ni++)
            ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
        memcpy(m_pfOverlap[niEar], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(audio_t));
    }
}

void AmbisonicBinauralizer::PrintTimeInfo(unsigned factor) {
    printf("---------------------------------------------\n");
    printf("TOTAL TIME FROM %s\n", Name);
    printf("---------------------------------------------\n");
    printf("Binaur FFT\t = %lu\n", TotalTime[0]/factor);
    printf("Binaur FIR\t = %lu\n", TotalTime[1]/factor);
    printf("Binaur IFFT\t = %lu\n", TotalTime[2]/factor);
    printf("\n");
}