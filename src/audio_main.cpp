#include <audio.hpp>

int main(int argc, char **argv) {
    const int numBlocks = NUM_BLOCKS;  
   
    ABAudio audio;
    audio.Configure();

    printf("[MAIN] Starting Processing\n");
    audio.loadSource();

    for (int i = 0; i < numBlocks; i++) {
        audio.processBlock();
        printf("[MAIN] Audio block %d done\n", i);
    }

    audio.PrintTimeInfo(numBlocks);

    while(1);

    return 0;
}