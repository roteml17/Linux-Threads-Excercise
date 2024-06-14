#ifndef BLOCKCHAIN__H
#define BLOCKCHAIN__H

#include <queue>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <pthread.h>
#include <zlib.h>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <thread>

using namespace std;

typedef struct 
{
    int height;
    int timeStamp;
    unsigned int hash;
    unsigned int prev_hash;
    int difficulty;
    int nonce;
    int relayed_by;
} BLOCK_T;

class blockChain
{
private:
    BLOCK_T lastBlock;
    BLOCK_T notMinedBlock;
    pthread_mutex_t mtx_lock;
    queue<BLOCK_T> blocks_queue;
    pthread_cond_t condition_variable;
    pthread_cond_t block_mined_condition; ////
    int miner_thread_id;
    int current_mining_height; //////

public:
     blockChain(int difficulty);
    ~blockChain() {}
    int calculateHash(const BLOCK_T& block);
    bool validationProofOfWork(int hash, int difficulty);
    void startMining();
    static void* serverThread(void* args); 
    static void* minerThread(void* args);
    void notifyMiners();
    bool isBlockValid(const BLOCK_T& block);
    static void* testMinerThread(void* args);
    pthread_mutex_t& getMutex();
    BLOCK_T getBlock();
    void changeNotMindBlock();
    void notifyBlockMined(); ////
};

#endif