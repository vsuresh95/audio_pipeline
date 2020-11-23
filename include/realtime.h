/*
 * Realtime audio support for ILLIXR's audio processing pipeline.
 *
 * The realtime audio support requires the PortAudio library.
 */

#ifndef REALTIME_H
#define REALTIME_H

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
void *illixr_rt_init(void *audioObj);

#endif // REALTIME_H
