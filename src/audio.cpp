#include <audio.hpp>
#include <cstring>

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
    // 2. Shared memory invocation chain
    if (DO_CHAIN_OFFLOAD) {
        // Configure accelerator parameters and write them to accelerator registers.
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);
        FFIChainInst.ConfigureAcc();

        // Assign the configured accelerator object to both rotator and decoder.
        rotator.FFIChainInst = FFIChainInst;
        decoder.FFIChainInst = FFIChainInst;

    	// Write input data for psycho twiddle factors
    	FFIChainInst.InitTwiddles(&sumBF, rotator.m_pFFT_psych_cfg->super_twiddles);
    } else if (DO_NP_CHAIN_OFFLOAD) {
        // Configure accelerator parameters and write them to accelerator registers,
        // and start the accelerators (to start polling).
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);
        FFIChainInst.ConfigureAcc();
        FFIChainInst.StartAcc();

        // Assign the configured accelerator object to both rotator and decoder.
        rotator.FFIChainInst = FFIChainInst;
        decoder.FFIChainInst = FFIChainInst;

    	// Write input data for psycho twiddle factors.
    	FFIChainInst.InitTwiddles(&sumBF, rotator.m_pFFT_psych_cfg->super_twiddles);
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
            sumBF.m_ppfChannels[niChannel][niSample] = myRand();
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
    StartCounter();
    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);
    EndCounter(1);

    // Perform binauralizer filtering to decode audio for 2 ears.
    StartCounter();
    decoder.Process(&sumBF, resultSample);
    EndCounter(2);
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
