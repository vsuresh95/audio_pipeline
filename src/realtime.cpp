#include <realtime.h>
#include <stdio.h>
#include <math.h>
#include <sound.h>
#include <audio.h>
#include "portaudio.h"

#define SAMPLE_RATE ((SAMPLERATE))
#define AUDIO_FORMAT   paInt16
typedef unsigned short          sample_t;
#define SILENCE       ((sample_t)0x00)

using namespace ILLIXR_AUDIO;

/*
 * illixr_rt_cb
 *
 * ILLIXR realtime audio callback method
 *
 * When the realtime audio driver requires more buffered data, this callback
 * method is called. This is where ILLIXR feeds more audio data to the realtime audio engine.
 */
static int illixr_rt_cb( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData ) {

    sample_t *out = (sample_t*)outputBuffer;
    ABAudio *audioObj = (ABAudio *)userData;
    int i;
    (void) inputBuffer; /* Prevent unused variable warnings. */

    // As this is an interrupt context, lots of data processing is ill-advised.
    // In future iterations, all processing will be handled in a separate thread with proper
    // synchronization primatives.
    audioObj->processBlock();

    for (i = 0; i < BLOCK_SIZE; i++) {
        *out++ = audioObj->mostRecentBlockL[i];
        *out++ = audioObj->mostRecentBlockR[i];
    }

    return 0;
}

/*******************************************************************/

/*
 * illixr_rt_init
 *
 * Initializes and launches the realtime audio thread, communicating with PortAudio
 * This method should be launched as a separate pthread.
 *
 * Arguments:
 * audioObj - A pointer to an ABAudio object from which audio will be streamed to the speakers.
 *
 * Returns:
 * PortAudio error codes cast to void * type
 *
 * Side Effects:
 * This function is blocking and will not return until the audio source is exhausted.
 * Launch this in an independent thread!
 */
void *illixr_rt_init(void *audioObj)
{
    PaStreamParameters  outputParameters;
    PaStream*           stream;
    PaError             err;
    PaTime              streamOpened;
    int                 i, totalSamps;

    printf("Initializing audio hardware...\n");

    err = Pa_Initialize();
    if( err != paNoError )
        goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* Default output device. */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;                     /* Stereo output. */
    outputParameters.sampleFormat = AUDIO_FORMAT;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream( &stream,
                         NULL,      /* No input. */
                         &outputParameters,
                         SAMPLE_RATE,
                         BLOCK_SIZE,       /* Frames per buffer. */
                         paClipOff, /* We won't output out of range samples so don't bother clipping them. */
                         illixr_rt_cb,
                         audioObj );
    if( err != paNoError )
        goto error;

    streamOpened = Pa_GetStreamTime( stream ); /* Time in seconds when stream was opened (approx). */

    printf("Launching stream!\n");
    err = Pa_StartStream( stream );
    if( err != paNoError )
        goto error;

    // Spin until the audio marks itself as being complete
    while( ((ABAudio *)audioObj)->num_blocks_left > 0) {
        Pa_Sleep(0.25f);
    }

    printf("Stopping stream\n");

    err = Pa_CloseStream( stream );
    if( err != paNoError )
        goto error;

    Pa_Terminate();
    return (void *)err;
error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return (void *)err;
}
