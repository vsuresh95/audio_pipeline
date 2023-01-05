#include <audio.hpp>
#include <cstring>

#if (USE_REAL_DATA == 1)
#include <m_ppfChannels.hpp>
#include <resultSample.hpp>
#endif // USE_REAL_DATA

void ABAudio::Configure() {
    Name = (char *) "AUDIO";

    // Process for rotate and psycho-acoustic filter.
    rotator.Configure(BLOCK_SIZE, NUM_SRCS);

    // Zoomer processor.
    zoomer.Configure(NUM_SRCS);

    // Binauralizer for ambisonic decoding.
    decoder.Configure(SAMPLERATE, BLOCK_SIZE, NUM_SRCS);

    // Allocate memory and initialize final result buffer.
    resultSample = (audio_t **) aligned_malloc(2 * sizeof(audio_t *));
    resultSample[0] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));
    resultSample[1] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));
    memset(resultSample[0], 0, BLOCK_SIZE * sizeof(audio_t));
    memset(resultSample[1], 0, BLOCK_SIZE * sizeof(audio_t));

    // Configure and initialize parameters for Bformat data object,
    // which holds the data for each channel throughout the pipeline.
    // This will be shared among all the tasks, and each task will read
    // and update data from and to this object, respectively.
    sumBF.Configure(BLOCK_SIZE, NUM_SRCS);

    // Hardware acceleration
    // 1. Regular invocation chain
    // 2. Shared memory invocation chain - non-pipelined or pipelined
    if (DO_CHAIN_OFFLOAD || DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        // Configure accelerator parameters and write them to accelerator registers.
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);
        FFIChainInst.ConfigureAcc();

        // Assign size parameters that is used for data store and load loops
        FFIChainInst.m_nChannelCount = rotator.m_nChannelCount;
        FFIChainInst.m_fFFTScaler = rotator.m_fFFTScaler;
        FFIChainInst.m_nBlockSize = rotator.m_nBlockSize;
        FFIChainInst.m_nFFTSize = rotator.m_nFFTSize;
        FFIChainInst.m_nFFTBins = rotator.m_nFFTBins;

        // Assign the configured accelerator object to both rotator and decoder.
        rotator.FFIChainInst = FFIChainInst;
        decoder.FFIChainInst = FFIChainInst;

    	// Write input data for psycho twiddle factors
    	FFIChainInst.InitTwiddles(&sumBF, rotator.m_pFFT_psych_cfg->super_twiddles);
    }
    
    // For shared memory invocation cases, we can
    // start the accelerators (to start polling).
    if (DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) {
        FFIChainInst.StartAcc();
    }
}

void ABAudio::loadSource() {
    // TODO: Set .fAzimuth, .fElevation and .fDistance for soundSrcs
    // using setSrcPos. For now, it's ignored.
}

void ABAudio::processBlock() {
    // Initialize audio source data. Since we're not reading an actual audio
    // source file, we initialize we random data.
    for(unsigned niChannel = 0; niChannel < sumBF.m_nChannelCount; niChannel++) {
        for(unsigned niSample = 0; niSample < sumBF.m_nSamples; niSample++) {
            #if (USE_REAL_DATA == 1)
            sumBF.m_ppfChannels[niChannel][niSample] = Orig_m_ppfChannels[niChannel][niSample];
            #else
            sumBF.m_ppfChannels[niChannel][niSample] = myRand();
            #endif
        }
    }

    // First, we update the rotation parameters. In the bare metal app, we use
    // random numbers. Then, we perform psycho-acoustic filtering and 3D rotation.
    StartCounter();
    rotator.updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);
    EndCounter(0);

    // First, we update the zoomer parameters. In the bare metal app, we use
    // random numbers. Then, we perform zoomer process.
	WriteScratchReg(0x88888888);
    StartCounter();
    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);
    EndCounter(1);
	WriteScratchReg(0);

    // Perform binauralizer filtering to decode audio for 2 ears.
    StartCounter();
    decoder.Process(&sumBF, resultSample);
    EndCounter(2);

#if (USE_REAL_DATA == 1)
    unsigned error_cnt = 0;
    
    print_float_t Actual; 
    print_float_t Expected;

    for(unsigned niChannel = 0; niChannel < 2; niChannel++) {
        for(unsigned niSample = 0; niSample < BLOCK_SIZE; niSample++) {
            Actual.Flt = resultSample[niChannel][niSample];
            Expected.Flt = Orig_resultSample[niChannel][niSample];

            if ((fabs(Expected.Flt - Actual.Flt) / fabs(Expected.Flt)) > ERR_TH) {
			    error_cnt++;
                printf("Error at %d %d A = %x, E = %x\n", niChannel, niSample, Actual.Int, Expected.Int);
            }
        }
    }

    if (error_cnt) {
        printf("TEST FAIL = %d\n", error_cnt);
    } else {
        printf("TEST PASS\n");
    }
#endif // USE_REAL_DATA

}

void ABAudio::PrintTimeInfo(unsigned factor) {
    printf("---------------------------------------------\n");
    printf("TOTAL TIME FROM %s\n", Name);
    printf("---------------------------------------------\n");
    printf("Psycho Filter\t = %lu\n", TotalTime[0]/factor);
    printf("Zoomer Process\t = %lu\n", TotalTime[1]/factor);
    printf("Binaur Filter\t = %lu\n", TotalTime[2]/factor);
    printf("\n");

    // Call lower-level print functions.
    rotator.PrintTimeInfo(factor);
    decoder.PrintTimeInfo(factor);
}
