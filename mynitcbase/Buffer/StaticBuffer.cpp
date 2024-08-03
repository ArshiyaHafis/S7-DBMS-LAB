#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false;
        metainfo[bufferIndex].timeStamp = -1;
        metainfo[bufferIndex].blockNum = -1;
    }
}

StaticBuffer::~StaticBuffer() 
{
    for (int bufferIndex = 0;bufferIndex< BUFFER_CAPACITY;bufferIndex++) {
        if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true){
            Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if (blockNum < 0 || blockNum > DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }
    int bufferNum = -1;

    for(int blocks = 0; blocks < BUFFER_CAPACITY; blocks++)
    {
        if (metainfo[blocks].free == false)
        {
            metainfo[blocks].timeStamp++;
        }
    }

    for (int bufferIndex = 0;bufferIndex< BUFFER_CAPACITY;bufferIndex++) {
        if(metainfo[bufferIndex].free == true){
            bufferNum = bufferIndex;
            break;
        }
    }

    if(bufferNum==-1)
    {
        int maxTimeStamp = -1;
        int bufferIndexWithMaxTimeStamp = -1;

        for (int bufferIndex = 0;bufferIndex< BUFFER_CAPACITY;bufferIndex++) {
            if(metainfo[bufferIndex].timeStamp > maxTimeStamp){
                maxTimeStamp = metainfo[bufferIndex].timeStamp;
                bufferIndexWithMaxTimeStamp = bufferIndex;
            }
        }
        if(metainfo[bufferIndexWithMaxTimeStamp].dirty == true){
            Disk::writeBlock(blocks[bufferIndexWithMaxTimeStamp],metainfo[bufferIndexWithMaxTimeStamp].blockNum);
        }
        bufferNum = bufferIndexWithMaxTimeStamp;
        
    }
    metainfo[bufferNum].free = false;
    metainfo[bufferNum].blockNum = blockNum;
    return bufferNum;
}


int StaticBuffer::getBufferNum(int blockNum) {
    if (blockNum < 0 || blockNum > DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }
    for (int blocks = 0; blocks < BUFFER_CAPACITY; blocks++) {
        if (metainfo[blocks].blockNum == blockNum)
            return blocks;
    }
  return E_BLOCKNOTINBUFFER;
}



int StaticBuffer::setDirtyBit(int blockNum){
    int bufferNum = StaticBuffer::getBufferNum(blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER){
        return E_BLOCKNOTINBUFFER;
    }
    if(bufferNum == E_OUTOFBOUND){
        return E_OUTOFBOUND;
    }
    metainfo[bufferNum].dirty = true;
    return SUCCESS;
}