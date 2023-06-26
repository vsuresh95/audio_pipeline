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

    printf("[MAIN] Starting Processing\n");
    audio.loadSource();

    // Process all audio blocks one by one.
    for (int i = 0; i < numBlocks; i++) {
        audio.processBlock();
        printf("[MAIN] %d done\n", i);
    }

    // Print out profile results.
    audio.PrintTimeInfo(numBlocks);

    while(1);

    return 0;
}

void PrintHeader() {
    printf("---------------------------------------------\n");
    printf("3D AUDIO DECODER\n");
    printf("---------------------------------------------\n");
    printf("NUM_BLOCKS = %d\n", NUM_BLOCKS);
    printf("BLOCK_SIZE = %d\n", BLOCK_SIZE);
    printf("COH PROTOCOL = %s\n", CohPrintHeader);
    printf("CONFIG = %s\n", USE_MONOLITHIC_ACC ? "Monolithic Accelerator" :
                            "Composable Accelerator");
    printf("OFFLOADING = %s\n", (DO_CHAIN_OFFLOAD ? "Regular Invocation" :
                                (DO_NP_CHAIN_OFFLOAD ? "Shared Memory Invocation" :
                                (DO_PP_CHAIN_OFFLOAD ? "Shared Memory Invocation - Pipelined" :
                                "No Offloading"))));
    printf("\n");
}