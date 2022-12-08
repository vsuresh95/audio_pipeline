#include <audio.hpp>

int main(int argc, char **argv) {
    const int numBlocks = NUM_BLOCKS;  
   
    printf("Test started\n");

    ABAudio audio;
    audio.Configure();

    printf("Audio Configure done\n");

    audio.loadSource();

    printf("Audio loadSource done\n");

    for (int i = 0; i < numBlocks; i++) {
        audio.processBlock();
    }

    return 0;
}