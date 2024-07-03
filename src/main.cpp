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

    // if (argc < 2) {
	// 	std::cout << "Usage: " << argv[0] << " <number of size 1024 blocks to process> ";
	// 	std::cout << "<optional: encode/decode>\n";
	// 	std::cout << "Note: If you want to hear the output sound, limit the process sample blocks so that the output is not longer than input!\n";
    //     return 1;
    // }

    int numBlocks = 50;
    if (argc > 1){
        numBlocks = atoi(argv[1]);
    }
    ABAudio::ProcessType procType(ABAudio::ProcessType::DECODE);
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

    for (int block = 0; block < numBlocks; block++) {
        // Process next block
        audio.processBlock();

        // Display a dynamic progress bar.
        int barWidth = 70;
        float progress = (float) (block + 1) / numBlocks;
        std::cout << "PROGRESS: [";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; i++) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << " %\r";
        std::cout.flush();
    }
    std::cout << std::endl;

    // Print out profile results.
    audio.PrintTimeInfo(numBlocks);

    return 0;
}

void PrintHeader() {
    printf("--------------------------------------------------------------------------------------\n");
    printf("APPLICATION:\t3D Spatial Audio\n");
    printf("SYSTEM:\t\t%s", (USE_MONOLITHIC_ACC) ? "Monolithic Accelerator" :"Disaggregated Accelerators");
    printf(" %s\n", (DO_PP_CHAIN_OFFLOAD) ? "Pipelined" : "");
    printf("INVOCATION:\t%s\n", (DO_NP_CHAIN_OFFLOAD || DO_PP_CHAIN_OFFLOAD) ? "Accelerator Synchronization Interface (ASI)" : "Linux (ioctl) invocation"); 
    printf("COHERENCE:\t%s\n", (IS_ESP) ? "MESI Coherence" : "Spandex Coherence"); 
    printf("--------------------------------------------------------------------------------------\n");
}
