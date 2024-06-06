#include "blockChain.h"

blockChain::blockChain(int difficulty)
{
    BLOCK_T block;
    block.height = 0;
    block.timeStamp = time(nullptr);
    block.hash = 0;
    block.prev_hash = 0;
    block.difficulty = difficulty;
    block.nonce = 0;
    block.relayed_by = -1;
    blocks.push_back(block);

    pthread_mutex_init(&mtx_lock, NULL);
    pthread_cond_init(&condition_variable, NULL);
}

int blockChain::calculateHash(const BLOCK_T& block)
{
    string input = to_string(block.height) +
                   to_string(block.timeStamp) +
                   to_string(block.prev_hash) +
                   to_string(block.nonce) +
                   to_string(block.relayed_by);
    return crc32(0,reinterpret_cast<const unsigned char*>(input.c_str()),input.length());
}

bool blockChain::validationProofOfWork(int hash, int difficulty)
{
    for(int i = 0; i < difficulty; ++i)
    {
        if((hash & (1 << (31 - i))) != 0) 
        {
            return false;
        }
    }

    return true;    
}

void blockChain::startMining()
{
    if (blocks.empty()) 
    {
        cout << "Error: Blockchain is empty!" << endl;
        return;
    }

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, &serverThread, (void*)this);

    for(int i = 1; i < 5; ++i)
    {   
        pthread_t miner_thread;
        miner_thread_id = i;
        pthread_create(&miner_thread, NULL, &minerThread, (void*)this);
        miners.push_back(miner_thread);
    }

    for(auto &miner : miners)
    {
        pthread_join(miner, NULL);
    }

    pthread_join(server_thread, NULL);
}

void* blockChain::minerThread(void* args)
{
    blockChain* block_chain = static_cast<blockChain*>(args);
    int miner_id = block_chain->miner_thread_id;
    while(true)
    {
        BLOCK_T newBlock;
        
        pthread_mutex_lock(&block_chain->mtx_lock); 
        newBlock = block_chain->blocks.back();
        pthread_mutex_unlock(&block_chain->mtx_lock);

        newBlock.relayed_by = miner_id; // Assign the thread ID as relayed_by
        newBlock.nonce = 0;

        while(true)
        {
            newBlock.timeStamp = time(nullptr);
            newBlock.hash = block_chain->calculateHash(newBlock);
            if(block_chain->validationProofOfWork(newBlock.hash, newBlock.difficulty))
            {
                pthread_mutex_lock(&block_chain->mtx_lock); 
                block_chain->blocks_queue.push(newBlock);
                pthread_cond_signal(&block_chain->condition_variable);
                pthread_mutex_unlock(&block_chain->mtx_lock);
                break;
            }

            newBlock.nonce++;
        }
    }

    return NULL;
}

void* blockChain::serverThread(void* args)
{
    blockChain* block_chain = static_cast<blockChain*>(args);

    while(true)
    {
        pthread_mutex_lock(&block_chain->mtx_lock); 

        while(block_chain->blocks_queue.empty())
        {
            pthread_cond_wait(&block_chain->condition_variable, &block_chain->mtx_lock);
        }

        BLOCK_T newBlock = block_chain->blocks_queue.front();
        block_chain->blocks_queue.pop();

        if(block_chain->isBlockValid(newBlock))
        {
            block_chain->blocks.push_back(newBlock);
            cout << "Miner #" << newBlock.relayed_by << ": Mined a new block #" << newBlock.height << " with the hash" << newBlock.hash <<endl;
            cout << "Server:"  << " New block added by " << newBlock.relayed_by << " attributes: height(" << newBlock.height << "), timestamp("<< newBlock.timeStamp
            << "), hash(" << newBlock.hash << "), prev_hash(" <<newBlock.prev_hash<< ") difficulty (" <<
            newBlock.difficulty << "), nonce (" << newBlock.nonce << ")" <<endl;

            block_chain->notifyMiners();
        }
        else
        {
            cout << "Invalid block received" << endl;
        }

        pthread_mutex_unlock(&block_chain->mtx_lock);
    }

    return NULL;
}

void blockChain::notifyMiners()
{
    pthread_cond_broadcast(&condition_variable);
}

bool blockChain::isBlockValid(const BLOCK_T& block)
{
    // Check if the block's prev_hash matches the hash of the previous block
    if (block.height > 0 && block.prev_hash != blocks[block.height - 1].hash)
    {
        return false;
    }

    // Check if the block's hash satisfies the required difficulty level
    return validationProofOfWork(block.hash, block.difficulty);
}

void* blockChain::testMinerThread(void* args)
{
    blockChain* block_chain = static_cast<blockChain*>(args);

    while(true)
    {
        BLOCK_T invalidBlock;

        // Create an invalid block (for testing purposes)
        invalidBlock.height = block_chain->getBlocks().size();
        invalidBlock.timeStamp = time(nullptr);
        invalidBlock.prev_hash = block_chain->getBlocks().back().hash;
        invalidBlock.difficulty = block_chain->getBlocks().back().difficulty;
        invalidBlock.nonce = 0;
        invalidBlock.relayed_by = 0; // Set relayed_by to a fixed value for identification

        // Send the invalid block to the server
        pthread_mutex_lock(&block_chain->mtx_lock); 
        block_chain->blocks_queue.push(invalidBlock);
        pthread_cond_signal(&block_chain->condition_variable);
        pthread_mutex_unlock(&block_chain->mtx_lock);

        // Sleep for 1 second before sending another invalid block
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return NULL;
}