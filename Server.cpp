#include "Server.h"

void Server::createBlockChain()
{
    while(true)
    {
        BLOCK_T new_block;
        
        vector<BLOCK_T> chain = block_chain->getBlocks();
        new_block.height = chain.size();
        new_block.timeStamp = time(nullptr);
        new_block.prev_hash = chain.back().hash; //last block on the chain
        new_block.difficulty = chain.back().difficulty;

        chain.push_back(new_block); 
    }
}
