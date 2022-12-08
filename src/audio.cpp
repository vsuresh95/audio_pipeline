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
    
    resultSample[0] = (float *) aligned_malloc(BLOCK_SIZE * sizeof(float));
    resultSample[1] = (float *) aligned_malloc(BLOCK_SIZE * sizeof(float));

    /// Temporary BFormat file to sum up ambisonics
    CBFormat sumBF;
    sumBF.Configure(BLOCK_SIZE, NUM_SRCS);

    printf("sumBF Configure done\n");

    rotator.updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);

    printf("rotator Process done\n");

    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);

    printf("zoomer Process done\n");

    decoder.Process(&sumBF, resultSample);

    printf("decoder Process done\n");

    aligned_free(resultSample);
}