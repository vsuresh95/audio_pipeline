#include <audio.hpp>

void ABAudio::Configure() {
    Name = (char *) "AUDIO";

    /// Processor to rotate
    rotator.Configure(BLOCK_SIZE, NUM_SRCS);

    printf("[%s] rotator Configure done\n", Name);

    /// Processor to zoom
    zoomer.Configure(NUM_SRCS);

    printf("[%s] zoomer Configure done\n", Name);

    /// Binauralizer as ambisonics decoder
    decoder.Configure(SAMPLERATE, BLOCK_SIZE, NUM_SRCS);

    printf("[%s] decoder Configure done\n", Name);
    
    resultSample = (audio_t **) aligned_malloc(2 * sizeof(audio_t *));
    resultSample[0] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));
    resultSample[1] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));

    sumBF.Configure(BLOCK_SIZE, NUM_SRCS);

    if (DO_CHAIN_OFFLOAD) {
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);
        FFIChainInst.ConfigureAcc();

        rotator.FFIChainInst = FFIChainInst;
        decoder.FFIChainInst = FFIChainInst;

    	// Write input data for psycho twiddle factors
    	FFIChainInst.InitTwiddles(&sumBF, rotator.m_pFFT_psych_cfg->super_twiddles);
    } else if (DO_NP_CHAIN_OFFLOAD) {
        FFIChainInst.logn_samples = (unsigned) log2(BLOCK_SIZE);
        FFIChainInst.ConfigureAcc();
        FFIChainInst.StartAcc();

        rotator.FFIChainInst = FFIChainInst;
        decoder.FFIChainInst = FFIChainInst;
    }
}

void ABAudio::loadSource() {
    /// TODO: Set .fAzimuth, .fElevation and .fDistance for soundSrcs
    /// using setSrcPos. For now, it's ignored.
}

void ABAudio::processBlock() {
    for(unsigned niChannel = 0; niChannel < sumBF.m_nChannelCount; niChannel++) {
        for(unsigned niSample = 0; niSample < sumBF.m_nSamples; niSample++) {
            sumBF.m_ppfChannels[niChannel][niSample] = rand() % 100;
        }
    }

    StartCounter();
    rotator.updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);
    EndCounter(0);

    StartCounter();
    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);
    EndCounter(1);

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

    rotator.PrintTimeInfo(factor);
    decoder.PrintTimeInfo(factor);
}