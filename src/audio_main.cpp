#include <audio.hpp>

int main(int argc, char **argv) {
    const int numBlocks = NUM_BLOCKS;  
   
    ABAudio audio;
    audio.Configure();

    audio.loadSource();

    for (int i = 0; i < numBlocks; i++) {
        printf("Audio block %d done\n", i);

        audio.processBlock();
    }

    return 0;
}