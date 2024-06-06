#include "blockChain.h"

int main()
{
    blockChain blocks(4);
    blocks.startMining();

    // Create and start the test miner thread
    pthread_t test_miner_thread;
    pthread_create(&test_miner_thread, NULL, &blocks.testMinerThread, (void*)&blocks);

    // Join the test miner thread
    pthread_join(test_miner_thread, NULL);

    return 0;
}