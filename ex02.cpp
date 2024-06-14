#include "blockChain.h"
#include <cstdlib>

int main(int argc, char* argv[])
{
    int difficulty = std::atoi(argv[1]); // Convert difficulty from string to int

    // Check if conversion was successful
    if (difficulty <= 0) {
        std::cout << "Invalid difficulty value" << std::endl;
        return 1;
    }

    blockChain blocks(difficulty);
    blocks.startMining();

    // Create and start the test miner thread
    pthread_t test_miner_thread;
    pthread_create(&test_miner_thread, NULL, &blocks.testMinerThread, (void*)&blocks);

    // Join the test miner thread
    pthread_join(test_miner_thread, NULL);

    return 0;
}
