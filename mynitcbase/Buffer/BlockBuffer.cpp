#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>


// the declarations for these functions can be found in "BlockBuffer.h"


BlockBuffer::BlockBuffer(char blockType){
    int blockTypeInt;
    if(blockType=='R'){
        blockTypeInt = REC;
    }
    else if(blockType=='I'){
        blockTypeInt = IND_INTERNAL;
    }
    else if(blockType=='L'){
        blockTypeInt = IND_LEAF;
    }
    else{
        blockTypeInt = UNUSED_BLK;
    }
    int blockNum = getFreeBlock(blockTypeInt);
    this->blockNum = blockNum;

    if (blockNum < 0 || blockNum >= DISK_BLOCKS)
        return;
}

BlockBuffer::BlockBuffer(int blockNum)
{
    this->blockNum = blockNum;  // initialise this.blockNum with the argument
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

RecBuffer::RecBuffer() : BlockBuffer('R'){}


int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
    int diff;
    (attrType == NUMBER)
        ? diff = attr1.nVal - attr2.nVal
        : diff = strcmp(attr1.sVal, attr2.sVal);
    if (diff > 0)
        return 1; // attr1 > attr2
    else if (diff < 0)
        return -1; //attr 1 < attr2
    else 
        return 0;
}

int BlockBuffer::getHeader(struct HeadInfo *head) {

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) 
    {
        return ret;   // return any errors that might have occured in the process
    }

  // ... (the rest of the logic is as in stage 2)

    memcpy(&head->reserved, bufferPtr + 28, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->pblock, bufferPtr + 4, 4);
    memcpy(&head->blockType, bufferPtr, 4);
  // ... (the rest of the logic is as in stage 2)
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) 
{
  // ...
    struct HeadInfo head;
    this->getHeader(&head);
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) 
    {
        return ret;
    }
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    int recordSize = attrCount * ATTR_SIZE;
    int slotNumRecordOffset = (HEADER_SIZE + slotCount) + (recordSize* slotNum);
    unsigned char *slotPointer = bufferPtr + slotNumRecordOffset;

    memcpy(rec, slotPointer, recordSize);
  // ... (the rest of the logic is as in stage 2
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER) {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }
    else
    {
		for (int i = 0; i < BUFFER_CAPACITY; i++)
        {
            if (StaticBuffer::metainfo[i].free == false)
            {
                StaticBuffer::metainfo[i].timeStamp++;
            }
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }
    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
      return ret;
    }

    struct HeadInfo head;
    getHeader(&head);

    int slotCount = head.numSlots;
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    memcpy(slotMap, slotMapInBuffer, slotCount);

    return SUCCESS;
}


int RecBuffer::setRecord(union Attribute* rec, int slotNum) {
    unsigned char* bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
        return ret;

    struct HeadInfo header;
    this->getHeader(&header);

    int numAttrs = header.numAttrs;
    int numSlots = header.numSlots;

    if (slotNum < 0 || slotNum >= numSlots)
        return E_OUTOFBOUND;
    
    int recordSize = numAttrs*ATTR_SIZE;
	int slotNumRecordOffset = (HEADER_SIZE + numSlots) + (recordSize * slotNum);
    unsigned char* recordPtr = bufferPtr + slotNumRecordOffset;

    memcpy(recordPtr, rec, recordSize);
    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS)
    {
        return ret;
    }
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    bufferHeader->numSlots = head->numSlots;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->pblock = head->pblock;
    bufferHeader->blockType = head->blockType;
    
    ret = StaticBuffer::setDirtyBit(this->blockNum);

    // if setDirtyBit() failed, return the error code
    if(ret != SUCCESS)
    {
        return ret;
    }
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS){
        return ret;
    }
    *((int32_t *)bufferPtr) = blockType;
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS){
        return ret;
    }
    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){
    int block = -1;
    for (int i = 0; i < DISK_BLOCKS; i++){
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
            block = i;
            break;
        }
    }
    if (block == -1){
        return E_DISKFULL;
    }
    this->blockNum = block;
    int bufferNum = StaticBuffer::getFreeBuffer(block);
    struct HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;

    int ret = setHeader(&head);

    if(ret != SUCCESS){
        printf("Error in setting header.\n");
        exit(1);
    }

    ret = setBlockType(blockType);
    if (ret != SUCCESS){
        printf("Error in setting block type.\n");
        exit(1);
    }


    return block;
}
int RecBuffer::setSlotMap(unsigned char* slotMap) {
    unsigned char* bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS)
        return ret;

    struct HeadInfo head;
    getHeader(&head);

    int slotCount = head.numSlots;

    unsigned char* slotMapInBuffer = bufferPtr + HEADER_SIZE;

    memcpy(slotMapInBuffer, slotMap, slotCount);

    return StaticBuffer::setDirtyBit(this->blockNum);

}


// int RecBuffer::setSlotMap(unsigned char *slotMap) {
//     unsigned char *bufferPtr;
//     int ret = loadBlockAndGetBufferPtr(&bufferPtr);
//     if (ret != SUCCESS){
//         return ret;
//     }

//     struct HeadInfo head;
//     ret = BlockBuffer::getHeader(&head);

//     if(ret != SUCCESS){
//         printf("Error in getting header.\n");
//         exit(1);
//     }

//     int numSlots = head.numSlots;
//     unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
//     memcpy(slotMapInBuffer, slotMap, numSlots);
//     ret = StaticBuffer::setDirtyBit(this->blockNum);
//     if (ret != SUCCESS){
//         return ret;
//     }
//     return SUCCESS;
// }

int BlockBuffer::getBlockNum(){
    return this->blockNum;
}