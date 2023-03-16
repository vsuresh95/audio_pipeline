#include <audio.h>
#include <iostream>
#include <realtime.h>
#include <pthread.h>

void PrintHeader();

bool can_use_audio_dma;

int main(int argc, char const *argv[])
{
    using namespace ILLIXR_AUDIO;

    PrintHeader();

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

    can_use_audio_dma = (USE_AUDIO_DMA == 1) && (procType == ABAudio::ProcessType::DECODE);
    
    ABAudio audio("output.wav", procType);
    audio.loadSource();
    audio.num_blocks_left = numBlocks;

    printf("[MAIN] Starting Processing\n");
    for (int i = 0; i < numBlocks; ++i) {
        audio.processBlock();
        printf("[MAIN] Audio block %d done\n", i);
    }

    // Print out profile results.
    audio.PrintTimeInfo(numBlocks);

    return 0;
}

void PrintHeader() {
    printf("---------------------------------------------\n");
    printf("3D AUDIO DECODER\n");
    printf("---------------------------------------------\n");
    printf("COH PROTOCOL = %s\n", CohPrintHeader);
    printf("OFFLOADING = %s\n", (DO_CHAIN_OFFLOAD ? "Regular Invocation" :
                                (DO_NP_CHAIN_OFFLOAD ? "Shared Memory Invocation" :
                                (DO_PP_CHAIN_OFFLOAD ? "Shared Memory Invocation - Pipelined" :
                                "No Offloading"))));
    printf("\n");
}
