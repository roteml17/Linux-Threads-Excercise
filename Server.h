#ifndef SERVER__H
#define SERVER__H

#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "blockChain.h"
using namespace std;

class Server
{
private:
    blockChain* block_chain;

public:
    Server(blockChain* input_block_chain) : block_chain(input_block_chain) {}
    ~Server() {} 
    void createBlockChain();
};
#endif 