#include <audio.hpp>

// Compiler arguments (passed through audio_pipeline.mk):
// NUM_BLOCKS: Number of audio blocks (16)
// BLOCK_SIZE: Number of samples in each audio (1024)
// SAMPLERATE: Unused
// NORDER: Order of audio decoding (3)
// NUM_SRCS: Number of sources, or channel of audio (16)
// COH_MODE: Specialization of ESP/SPX coherence protocol (0)
// IS_ESP: Use ESP caches or Spandex caches (1)
// DO_CHAIN_OFFLOAD: Offload FFT-FIR-IFFT chain to accelerators, with regular invocation (0)
// DO_NP_CHAIN_OFFLOAD: Offload FFT-FIR-IFFT chain to accelerators, with SM invocation (0)
// USE_INT: Use int type for all data, or float for CPU data and fixed point for accelerators (1)

void PrintHeader();

int main(int argc, char **argv) {
    PrintHeader();

    const int numBlocks = NUM_BLOCKS;

    ABAudio audio;
    // Configure all tasks in the pipeline,
    // and initialize any data.
    audio.Configure();

    audio.loadSource();

    // Process all audio blocks one by one.
    for (int i = 0; i < numBlocks; i++) {
        audio.processBlock();
        printf("Block %d done\n", i);
    }

    // Print out profile results.
    audio.PrintTimeInfo(numBlocks);

    while(1);

    return 0;
}

void PrintHeader() {
    printf("--------------------------------------------------------------------------------------\n");
    printf("3D SPATIAL AUDIO DECODER: ");
    printf("%s\n", (DO_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Monolithic Accelerator for FFT-FIR-IFFT" :"Composed Fine-Grained Accelerators for FFT-FIR-IFFT") :
                                (DO_NP_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Monolithic Accelerator with ASI" :"Composed Fine-Grained Accelerators with ASI") :
                                (DO_PP_CHAIN_OFFLOAD ? ((USE_MONOLITHIC_ACC)? "Hardware Pipelining" : "Software Pipelining" ):
                                (DO_FFT_IFFT_OFFLOAD) ? "Hardware Acceleration of FFT-IFFT in EPOCHS" :
                                "All Software in EPOCHS"))));

    printf("--------------------------------------------------------------------------------------\n");
}