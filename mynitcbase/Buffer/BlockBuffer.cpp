#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>


// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum)
{
    this->blockNum = blockNum;  // initialise this.blockNum with the argument
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
// the declarations for these functions can be found in "BlockBuffer.h"


/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
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

/*
Used to get the record at slot `slotNum` into the array `rec`
NOTE: this function expects the caller to allocate memory for `rec`
*/
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

    unsigned char buffer[BLOCK_SIZE];   // read the block at this.blockNum into a buffer
    Disk::readBlock(buffer, this->blockNum);

    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */
    int recordSize = attrCount * ATTR_SIZE;
    int slotNumRecordOffset = (HEADER_SIZE + slotCount) + (recordSize* slotNum);
    unsigned char *slotPointer = buffer + slotNumRecordOffset;
    
    memcpy(rec, slotPointer, recordSize);
  // ... (the rest of the logic is as in stage 2
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}