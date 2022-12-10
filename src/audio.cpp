#include <audio.hpp>

void ABAudio::Configure() {
    /// Processor to rotate
    rotator.Configure(BLOCK_SIZE, NUM_SRCS);

    printf("[Audio] rotator Configure done\n");

    /// Processor to zoom
    zoomer.Configure(NUM_SRCS);

    printf("[Audio] zoomer Configure done\n");

    /// Binauralizer as ambisonics decoder
    decoder.Configure(SAMPLERATE, BLOCK_SIZE, NUM_SRCS);

    printf("[Audio] decoder Configure done\n");
}

void ABAudio::loadSource() {
    /// TODO: Set .fAzimuth, .fElevation and .fDistance for soundSrcs
    /// using setSrcPos. For now, it's ignored.
}

void ABAudio::processBlock() {
    audio_t **resultSample;
    
    resultSample = (audio_t **) aligned_malloc(2 * sizeof(audio_t *));
    resultSample[0] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));
    resultSample[1] = (audio_t *) aligned_malloc(BLOCK_SIZE * sizeof(audio_t));

    sumBF.Configure(BLOCK_SIZE, NUM_SRCS);

    rotator.updateRotation();
    rotator.Process(&sumBF, BLOCK_SIZE);

    zoomer.updateZoom();
    zoomer.Process(&sumBF, BLOCK_SIZE);

    decoder.Process(&sumBF, resultSample);

    aligned_free(resultSample);
}