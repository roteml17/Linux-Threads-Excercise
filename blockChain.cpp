#include "blockChain.h"
#include "Server.h"

blockChain::blockChain(int difficulty): current_mining_height(0)
{
    notMinedBlock.height = 0;
    notMinedBlock.timeStamp = time(nullptr);
    notMinedBlock.prev_hash = 0;
    notMinedBlock.difficulty = difficulty;
    notMinedBlock.nonce = 0;
    notMinedBlock.relayed_by = -1;
    notMinedBlock.hash = notMinedBlock.hash = calculateHash(notMinedBlock);

    pthread_mutex_init(&mtx_lock, NULL);
    pthread_cond_init(&condition_variable, NULL);
    pthread_cond_init(&block_mined_condition, NULL); /////
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
    vector<pthread_t> miners;
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

    while (true)
    {
        pthread_mutex_lock(&block_chain->mtx_lock);
        BLOCK_T newBlock = block_chain->getBlock();
        pthread_mutex_unlock(&block_chain->mtx_lock);

        while (true)
        {
            pthread_mutex_lock(&block_chain->mtx_lock);
            if (newBlock.height != block_chain->notMinedBlock.height)
            {
                pthread_mutex_unlock(&block_chain->mtx_lock);
                break;
            }
            pthread_mutex_unlock(&block_chain->mtx_lock);

            newBlock.relayed_by = miner_id; // Assign the thread ID as relayed_by
            newBlock.timeStamp = time(nullptr);
            int temp = block_chain->calculateHash(newBlock);

            if (block_chain->validationProofOfWork(temp, newBlock.difficulty))
            {
                pthread_mutex_lock(&block_chain->mtx_lock);
                if (newBlock.height == block_chain->notMinedBlock.height && newBlock.height == block_chain->current_mining_height) ///////
                {
                    newBlock.hash = temp;
                    block_chain->blocks_queue.push(newBlock);
                    pthread_cond_signal(&block_chain->condition_variable);
                    block_chain->notifyBlockMined();

                    cout << "Miner #" << std::dec << miner_id << ": Mined a new block #" << newBlock.height << ", with the hash " << std::showbase << std::hex << newBlock.hash << endl;
                    cout << "New block attributes: height(" << std::dec << newBlock.height << "), timestamp(" << newBlock.timeStamp
                         << "), hash(" << std::showbase << std::hex << newBlock.hash << "), prev_hash(" << std::showbase << std::hex << newBlock.prev_hash << "), difficulty ("
                         << std::dec << newBlock.difficulty << "), nonce (" << newBlock.nonce << ")" << endl;

                    // Wait for notification that a new block has been mined and added
                    pthread_cond_wait(&block_chain->block_mined_condition, &block_chain->mtx_lock);
                }
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
            // Wait for new blocks to be mined
            pthread_cond_wait(&block_chain->condition_variable, &block_chain->mtx_lock);
        }

        BLOCK_T newBlock = block_chain->blocks_queue.front();
        block_chain->blocks_queue.pop();

        if(block_chain->isBlockValid(newBlock)) //line 125
        {
            block_chain->lastBlock=newBlock; //line 127

            sleep(1);
            block_chain->changeNotMindBlock();

            newBlock = block_chain->notMinedBlock;
            

            block_chain->notifyMiners(); //line 140
        }
        else
        {
            cout << "Invalid block received by miner #" << std::dec << newBlock.relayed_by << ", height(" << newBlock.height << "), hash(" 
            << std::showbase << std::hex << newBlock.hash << "), prev_hash(" 
            << std::showbase << std::hex << newBlock.prev_hash << ")" << endl;        
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
    if (block.height > 0 && block.prev_hash != lastBlock.hash) ////
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
        invalidBlock.height = block_chain->lastBlock.height + 1;
        invalidBlock.timeStamp = time(nullptr);
        invalidBlock.prev_hash = block_chain->lastBlock.hash;
        invalidBlock.difficulty = block_chain->lastBlock.difficulty;
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
    notMinedBlock.height = lastBlock.height + 1;
    notMinedBlock.timeStamp = time(nullptr);
    notMinedBlock.prev_hash = lastBlock.hash; //last block on the chain
    notMinedBlock.difficulty = lastBlock.difficulty;
    current_mining_height = notMinedBlock.height; ///////

}

void blockChain::notifyBlockMined()
{
    pthread_cond_broadcast(&block_mined_condition);
}