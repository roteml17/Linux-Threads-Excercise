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
    vector<BLOCK_T> blocks;
    vector<pthread_t> miners;
    pthread_mutex_t mtx_lock;
    queue<BLOCK_T> blocks_queue;
    pthread_cond_t condition_variable;
    int miner_thread_id;

public:
    blockChain(int difficulty);
    ~blockChain() {}
    vector<BLOCK_T> getBlocks() { return blocks; }
    int calculateHash(const BLOCK_T& block);
    bool validationProofOfWork(int hash, int difficulty);
    void startMining();
    static void* serverThread(void* args);
    static void* minerThread(void* args);
    void notifyMiners();
    bool isBlockValid(const BLOCK_T& block);
    static void* testMinerThread(void* args);
};

#endif