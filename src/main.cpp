#include <audio.h>
#include <iostream>
#include <realtime.h>
#include <pthread.h>

int main(int argc, char const *argv[])
{
    using namespace ILLIXR_AUDIO;

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <number of size 1024 blocks to process> ";
        std::cout << "<optional: encode/decode>\n";
        std::cout << "Note: If you want to hear the output sound, limit the process sample blocks so that the output is not longer than input!\n";
        return 1;
    }

    const int numBlocks = atoi(argv[1]);
    ABAudio::ProcessType procType(ABAudio::ProcessType::FULL);
    if (argc > 2){
        if (!strcmp(argv[2], "encode"))
            procType = ABAudio::ProcessType::ENCODE;
        else
            procType = ABAudio::ProcessType::DECODE;
    }
    
    ABAudio audio("output.wav", procType);
    audio.loadSource();
    audio.num_blocks_left = numBlocks;

    // Launch realtime audio thread for audio processing
    if (procType == ABAudio::ProcessType::FULL) {
        pthread_t rt_audio_thread;
        pthread_create(&rt_audio_thread, NULL, illixr_rt_init, (void *)&audio);
        pthread_join(rt_audio_thread, NULL);
    }
    else {
        for (int i = 0; i < numBlocks; ++i) {
            audio.processBlock();
        }
    }

    return 0;
}
