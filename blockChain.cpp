#include "blockChain.h"
#include "Server.h"

blockChain::blockChain(int difficulty)
{
    notMinedBlock.height = 0;
    notMinedBlock.timeStamp = time(nullptr);
    notMinedBlock.hash = 0;                  //calc the hash
    notMinedBlock.prev_hash = 0;
    notMinedBlock.difficulty = difficulty;
    notMinedBlock.nonce = 0;
    notMinedBlock.relayed_by = -1;

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
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, &serverThread, (void*)this);

    for(int i = 1; i < 5; ++i)
    {   
        pthread_t miner_thread;
        miner_thread_id = i;
        pthread_create(&miner_thread, NULL, &minerThread, (void*)this);
        miners.push_back(miner_thread);
    }

    // Wait for miners to finish their work
    for(auto &miner : miners)
    {
        pthread_join(miner, NULL);
    }

    // Signal the server thread to exit
    pthread_cancel(server_thread);
}

void* blockChain::minerThread(void* args)
{
    blockChain* block_chain = static_cast<blockChain*>(args);
    int miner_id = block_chain->miner_thread_id;
    int temp;

    while(true)
    {
        BLOCK_T newBlock;
        pthread_mutex_lock(&block_chain->mtx_lock); 

        newBlock = block_chain->getBlock();
        pthread_mutex_unlock(&block_chain->mtx_lock);

        while(true)
        {
            if()
            {
                 newBlock = block_chain->getBlock();
            }
            while (block_chain->validationProofOfWork(newBlock.hash, newBlock.difficulty))
            {            
            pthread_mutex_lock(&block_chain->mtx_lock); 
            newBlock.relayed_by = miner_id; // Assign the thread ID as relayed_by
            newBlock.timeStamp = time(nullptr);
            temp = block_chain->calculateHash(newBlock);
            if(block_chain->validationProofOfWork(temp, newBlock.difficulty))
            {

                newBlock.hash = temp;
                block_chain->blocks_queue.push(newBlock);
                pthread_cond_signal(&block_chain->condition_variable);
                //break;
            }
            newBlock.nonce++;
            pthread_mutex_unlock(&block_chain->mtx_lock);
            }
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
            // Wait for new blocks to be mined
            pthread_cond_wait(&block_chain->condition_variable, &block_chain->mtx_lock);
        }

        BLOCK_T newBlock = block_chain->blocks_queue.front();
        block_chain->blocks_queue.pop();

        ////////////////move the prints to the minerthread
        if(block_chain->isBlockValid(newBlock))
        {
            block_chain->blocks.push_back(newBlock);
            
            cout << "Miner #" << std::dec << newBlock.relayed_by << ": Mined a new block #" << std::dec << newBlock.height << ", with the hash " << std::showbase << std::hex << newBlock.hash << endl;
            cout << "Server:"  << " New block added by " << std::dec << newBlock.relayed_by << ", attributes: height(" << std::dec << newBlock.height << "), timestamp("<< std::dec << newBlock.timeStamp
            << "), hash(" <<std::showbase << std::hex << newBlock.hash << "), prev_hash(" << std::showbase << std::hex << newBlock.prev_hash<< ") difficulty (" <<
            std::dec << newBlock.difficulty << "), nonce (" << std::dec << newBlock.nonce << ")" <<endl;

            sleep(1);
            block_chain->changeNotMindBlock();

            newBlock = block_chain->notMinedBlock;
            

            block_chain->notifyMiners();
        }
        else
        {
            cout << "Invalid block received" << endl; //add info
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

pthread_mutex_t& blockChain::getMutex()
{
        return mtx_lock;
}

BLOCK_T blockChain::getBlock()
{
    return notMinedBlock; 
}

void blockChain::changeNotMindBlock()
{
    notMinedBlock.nonce = 0;
    notMinedBlock.height = blocks.size();
    notMinedBlock.timeStamp = time(nullptr);
    notMinedBlock.prev_hash = blocks.back().hash; //last block on the chain
    notMinedBlock.difficulty = blocks.back().difficulty;
}