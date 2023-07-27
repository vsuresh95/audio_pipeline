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

    for (int i = 0; i < numBlocks; ++i) {
        // std::cout << "{" << std::endl;
        audio.processBlock();
        printf("Block %d done\n", i);
        // std::cout << "}," << std::endl;
    }

    // Print out profile results.
    audio.PrintTimeInfo(numBlocks);

    return 0;
}

void PrintHeader() {
    printf("--------------------------------------------------------------------------------------\n");
    printf("3D SPATIAL AUDIO DECODER: ");
//    printf("COHERENCE PROTOCOL = %s\n", CohPrintHeader);
    printf("%s\n", (DO_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Monolithic Accelerator for FFT-FIR-IFFT" :"Composed Fine-Grained Accelerators for FFT-FIR-IFFT") :
                                (DO_NP_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Monolithic Accelerator with ASI" :"Composed Fine-Grained Accelerators with ASI") :
                                (DO_PP_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Hardware Pipelining" : "Software Pipelining" ):
                                (DO_FFT_IFFT_OFFLOAD) ? "Hardware Acceleration of FFT-IFFT in EPOCHS" :
                                (DO_ROTATE_OFFLOAD) ? "Hardware Acceleration of Rotate Order in EPOCHS" :
                                "All Software in EPOCHS"))));

    printf("--------------------------------------------------------------------------------------\n");
}
