#include <stdio.h>
#include <cstring>
#include <AmbisonicBinauralizer.hpp>
#include <ReadWriteCodeHelper.hpp>

#if (USE_REAL_DATA == 1)
#include <m_ppcpFilters.hpp>
#endif // USE_REAL_DATA

void AmbisonicBinauralizer::Configure(unsigned nSampleRate, unsigned nBlockSize, unsigned nChannels) {
    Name = (char *) "BINAURALIZER";

    m_nChannelCount = nChannels;

    // Hard-coded based on observations from Linux app.
    if (USE_REAL_DATA == 1) {
        m_nTaps = 140;
    } else {
        m_nTaps = 141;
    }
    
    m_nBlockSize = nBlockSize;

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

    // Scratch buffer to hold the time domain samples - set to 0.
    // BufferB is used for individual channels. BufferA is used to
    // sum BufferB from each channel.
    m_pfScratchBufferA = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));
    m_pfScratchBufferB = (audio_t *) aligned_malloc(m_nFFTSize * sizeof(audio_t));
    MyMemset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(audio_t));
    MyMemset(m_pfScratchBufferB, 0, m_nFFTSize * sizeof(audio_t));

    // Allocate buffers for overlap operation.
    // In binauralizer filter, the overlap operation stores overlap
    // information that carry forward from each audio block to the
    // next, for each ear only.
    m_pfOverlap = (audio_t **) aligned_malloc(2 * sizeof(audio_t *));
    m_pfOverlap[0] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));
    m_pfOverlap[1] = (audio_t *) aligned_malloc(m_nOverlapLength * sizeof(audio_t));
    MyMemset(m_pfOverlap[0], 0, m_nOverlapLength * sizeof(audio_t));
    MyMemset(m_pfOverlap[1], 0, m_nOverlapLength * sizeof(audio_t));

    // Allocate FFT and iFFT for new size
    m_pFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    // Scratch buffer to hold the frequency domain samples - set to 0.
    m_pcpScratch = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
    MyMemset(m_pcpScratch, 0, m_nFFTBins * sizeof(kiss_fft_cpx));

    // Allocate space for filters for FIR.
    m_ppcpFilters = (kiss_fft_cpx ***) aligned_malloc(2 * sizeof(kiss_fft_cpx **));
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        m_ppcpFilters[niEar] = (kiss_fft_cpx **) aligned_malloc(m_nChannelCount * sizeof(kiss_fft_cpx *));
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            m_ppcpFilters[niEar][niChannel] = (kiss_fft_cpx *) aligned_malloc(m_nFFTBins * sizeof(kiss_fft_cpx));
        }
    }

    // Initialize Binauralizer filter values. We intitialize random data
    // instead of calculating them as in the Linux app.
#if (DO_DATA_INIT == 1)
    for(unsigned niEar = 0; niEar < 2; niEar++) {
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++) {
            for(unsigned niSample = 0; niSample < m_nFFTBins; niSample++) {
            #if (USE_REAL_DATA == 1)
                m_ppcpFilters[niEar][niChannel][niSample].r = Orig_m_ppcpFilters[niEar][niChannel][2*niSample];
                m_ppcpFilters[niEar][niChannel][niSample].i = Orig_m_ppcpFilters[niEar][niChannel][2*niSample+1];
            #else
                m_ppcpFilters[niEar][niChannel][niSample].r = myRand();
                m_ppcpFilters[niEar][niChannel][niSample].i = myRand();
            #endif
            }
        }
    }
#endif
}

void AmbisonicBinauralizer::Process(CBFormat *pBFSrc, audio_t **ppfDst) {
    unsigned niEar = 0;
    unsigned niChannel = 0;
    unsigned ni = 0;
    kiss_fft_cpx cpTemp;

    // Perform the convolution on both ears. Potentially more realistic results but requires double the number of
    // convolutions.

    // Offload the entire task to be pipelined using shared memory.
    if (DO_PP_CHAIN_OFFLOAD) {
        StartCounter();
        FFIChainInst.m_nOverlapLength = m_nOverlapLength;
        if (USE_AUDIO_DMA) {
            FFIChainInst.BinaurProcessDMA(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
        } else {
            FFIChainInst.BinaurProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
        }
        EndCounter(0);
    } else if (DO_CHAIN_OFFLOAD) {
        StartCounter();
        FFIChainInst.m_nOverlapLength = m_nOverlapLength;
        FFIChainInst.BinaurRegularProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
        EndCounter(0);
    } else if (DO_NP_CHAIN_OFFLOAD) {
        StartCounter();
        FFIChainInst.m_nOverlapLength = m_nOverlapLength;
        FFIChainInst.BinaurNonPipelineProcess(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap);
        EndCounter(0);
    } else if (DO_FFT_IFFT_OFFLOAD) {
        StartCounter();
        FFIChainInst.m_nOverlapLength = m_nOverlapLength;
        FFIChainInst.BinaurRegularFFTIFFT(pBFSrc, ppfDst, m_ppcpFilters, m_pfOverlap, m_pFFT_cfg, m_pIFFT_cfg);
        EndCounter(0);
    } else {
        kiss_fft_cpx cpTemp;

        // Pointers/structures to manage input/output data
        audio_token_t SrcData;
        audio_t* src;
        audio_token_t DstData;
        audio_t* dst;
        audio_token_t OverlapData;
        audio_t* overlap_dst;

        unsigned InitLength = m_nBlockSize;
        unsigned ZeroLength = m_nFFTSize - m_nBlockSize;
        unsigned ReadLength = m_nBlockSize;
        unsigned OverlapLength = round_up(m_nOverlapLength, 2);

        for(niEar = 0; niEar < 2; niEar++)
        {
            for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            {
                src = pBFSrc->m_ppfChannels[niChannel];
                dst = m_pfScratchBufferB;

                StartCounter();
                // Copying from pBFSrc->m_ppfChannels[niChannel] to m_pfScratchBufferB
                for (unsigned niSample = 0; niSample < InitLength; niSample+=2, src+=2, dst+=2)
                {
                    // Need to cast to void* for extended ASM code.
                    SrcData.value_64 = read_mem_reqv((void *) src);
                    write_mem_wtfwd((void *) dst, SrcData.value_64);
                }

                // Zeroing rest of m_pfScratchBufferB
                for (unsigned niSample = 0; niSample < ZeroLength; niSample+=2, dst+=2)
                {
                    // Need to cast to void* for extended ASM code.
                    SrcData.value_64 = 0;
                    write_mem_wtfwd((void *) dst, SrcData.value_64);
                }
                EndCounter(0);

                StartCounter();
                unsigned long long FFTPostProcTime = kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);
                EndCounter(1);

                StartCounter();
                for(ni = 0; ni < m_nFFTBins; ni++)
                {
                    C_MUL(cpTemp , m_pcpScratch[ni] , m_ppcpFilters[niEar][niChannel][ni]);
                    m_pcpScratch[ni] = cpTemp;
                }
                EndCounter(2);

                StartCounter();
                unsigned long long IFFTPreProcTime = kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);
                EndCounter(3);

                StartCounter();
                if (niChannel == m_nChannelCount - 1)
                {
                    src = m_pfScratchBufferB;
                    dst = ppfDst[niEar];
                    overlap_dst = m_pfOverlap[niEar];

                    // First, we copy the output, scale it, sum it to the earlier sum,
                    // and account for the overlap data from the previous block, for the same channel.
                    for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
                    {
                        // Need to cast to void* for extended ASM code.
                        SrcData.value_64 = read_mem_reqodata((void *) src);
                        OverlapData.value_64 = read_mem_reqv((void *) overlap_dst);
                        DstData.value_64 = read_mem_reqv((void *) dst);

                        DstData.value_32_1 = OverlapData.value_32_1 + m_fFFTScaler * (DstData.value_32_1 + SrcData.value_32_1);
                        DstData.value_32_2 = OverlapData.value_32_2 + m_fFFTScaler * (DstData.value_32_2 + SrcData.value_32_2);

                        // Need to cast to void* for extended ASM code.
                        write_mem_wtfwd((void *) dst, DstData.value_64);
                    }

                    // Second, we simply copy the output (with scaling), sum it
                    // with the previous outputs, as we are outside the overlap range.
                    for (unsigned niSample = OverlapLength; niSample < ReadLength; niSample+=2, src+=2, dst+=2)
                    {
                        // Need to cast to void* for extended ASM code.
                        SrcData.value_64 = read_mem_reqodata((void *) src);
                        DstData.value_64 = read_mem_reqv((void *) dst);

                        DstData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + SrcData.value_32_1);
                        DstData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + SrcData.value_32_2);

                        // Need to cast to void* for extended ASM code.
                        write_mem_wtfwd((void *) dst, DstData.value_64);
                    }

                    overlap_dst = m_pfOverlap[niEar];

                    // Last, we copy our output (with scaling), sum it with the
                    // previous output, directly to the overlap buffer only.
                    // This data will be used in the first loop for the next audio block.
                    for (unsigned niSample = 0; niSample < OverlapLength; niSample+=2, src+=2, dst+=2, overlap_dst+=2)
                    {
                        // Need to cast to void* for extended ASM code.
                        SrcData.value_64 = read_mem_reqodata((void *) src);
                        DstData.value_64 = read_mem_reqv((void *) dst);

                        OverlapData.value_32_1 = m_fFFTScaler * (DstData.value_32_1 + SrcData.value_32_1);
                        OverlapData.value_32_2 = m_fFFTScaler * (DstData.value_32_2 + SrcData.value_32_2);

                        // Need to cast to void* for extended ASM code.
                        write_mem_wtfwd((void *) overlap_dst, OverlapData.value_64);
                    }
                } else if (niChannel == 0) {
                    src = m_pfScratchBufferB;
                    dst = ppfDst[niEar];

                    // Here, we simply copy the output, sum it with the previous outputs.
                    for (unsigned niSample = 0; niSample < ReadLength + OverlapLength; niSample+=2, src+=2, dst+=2)
                    {
                        // Need to cast to void* for extended ASM code.
                        SrcData.value_64 = read_mem_reqodata((void *) src);
                        write_mem_wtfwd((void *) dst, SrcData.value_64);
                    }
                } else {
                    src = m_pfScratchBufferB;
                    dst = ppfDst[niEar];

                    // Here, we simply copy the output, sum it with the previous outputs.
                    for (unsigned niSample = 0; niSample < ReadLength + OverlapLength; niSample+=2, src+=2, dst+=2)
                    {
                        // Need to cast to void* for extended ASM code.
                        SrcData.value_64 = read_mem_reqodata((void *) src);
                        DstData.value_64 = read_mem_reqv((void *) dst);

                        DstData.value_32_1 = DstData.value_32_1 + SrcData.value_32_1;
                        DstData.value_32_2 = DstData.value_32_2 + SrcData.value_32_2;

                        // Need to cast to void* for extended ASM code.
                        write_mem_wtfwd((void *) dst, DstData.value_64);
                    }
                }
                EndCounter(4);

                TotalTime[1] -= FFTPostProcTime;
                TotalTime[2] += FFTPostProcTime + IFFTPreProcTime;
                TotalTime[3] -= IFFTPreProcTime;
            }
        }
    }
}

void AmbisonicBinauralizer::PrintTimeInfo(unsigned factor) {
    printf("\n");
    printf("---------------------------------------------\n");
    printf("BINAURALIZER TIME\n");
    printf("---------------------------------------------\n");
    printf("Total Time\t\t = %llu\n", (TotalTime[0] + TotalTime[1] + TotalTime[2] + TotalTime[3] + TotalTime[4])/factor);

    if (!(DO_FFT_IFFT_OFFLOAD || DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD)) {
        printf("\n");
        printf("Binaur Init Data\t = %llu\n", TotalTime[0]/factor);
        printf("Binaur FFT\t\t = %llu\n", TotalTime[1]/factor);
        printf("Binaur FIR\t\t = %llu\n", TotalTime[2]/factor);
        printf("Binaur IFFT\t\t = %llu\n", TotalTime[3]/factor);
        printf("Binaur Overlap\t\t = %llu\n", TotalTime[4]/factor);
    }
}