#include <audio.hpp>

void ABAudio::Configure() {
    /// Processor to rotate
    rotator.Configure(BLOCK_SIZE, NUM_SRCS);

    printf("rotator Configure done\n");

    /// Processor to zoom
    zoomer.Configure(NUM_SRCS);

    printf("zoomer Configure done\n");

    /// Binauralizer as ambisonics decoder
    decoder.Configure(SAMPLERATE, BLOCK_SIZE, NUM_SRCS);

    printf("decoder Configure done\n");
}

void ABAudio::loadSource() {
    /// TODO: Set .fAzimuth, .fElevation and .fDistance for soundSrcs
    /// using setSrcPos. For now, it's ignored.
}

void ABAudio::processBlock() {
    float **resultSample;
    
    resultSample = (float **) aligned_malloc(2 * sizeof(float *));
    resultSample[0] = (float *) aligned_malloc(BLOCK_SIZE * sizeof(float));
    resultSample[1] = (float *) aligned_malloc(BLOCK_SIZE * sizeof(float));

    /// Temporary BFormat file to sum up ambisonics
    CBFormat sumBF;
    sumBF.Configure(BLOCK_SIZE, NUM_SRCS);

    rotator.updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);

    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);

    decoder.Process(&sumBF, resultSample);

    aligned_free(resultSample);
}