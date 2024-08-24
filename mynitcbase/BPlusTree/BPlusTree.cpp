#include "BPlusTree.h"
#include <iostream>
#include <cstring>

using std::cout;
using std::endl;

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1) 
    {
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1) {
            return RecId{-1, -1};
        }

    } 
    else 
    {

        block = searchIndex.block;
        index = searchIndex.index + 1;  
        IndLeaf leaf(block);
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            block = leafHead.rblock;
            index = 0;

            if (block == -1) {
                return RecId{-1, -1};
            }
        }
    }

    while(StaticBuffer::getStaticBlockType(block) == IND_INTERNAL) { 
        IndInternal internalBlk(block);
        HeadInfo intHead;

        internalBlk.getHeader(&intHead);
        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE) 
        {
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;

        } 
        else 
        {
            int i = 0;
            while (i < intHead.numEntries) {
                internalBlk.getEntry(&intEntry, i);
                int cval = compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType);
                
                if (((op == EQ || op == GE) && cval >= 0) ||(op == GT && cval > 0))
                {
                    break;
                }
                i++;
            }

            if (i < intHead.numEntries) {
                block = intEntry.lChild;
            } 
            else 
            {
                block = intEntry.rChild;
            }
        }
    }

    while (block != -1) {
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);
        Index leafEntry;

        while (index < leafHead.numEntries) 
        {
            leafBlk.getEntry(&leafEntry, index);
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                IndexId searchIndex{block, index};
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                return RecId{leafEntry.block, leafEntry.slot};

            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                return RecId{-1, -1};
            }
            ++index;
        }

        if (op != NE) {
            break;
        }
        block = leafHead.rblock;
        index = 0;
    }
    return RecId{-1, -1};
}


int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE]) 
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;
    
    AttrCatEntry attrCatEntryBuffer;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntryBuffer);
    
    if (ret != SUCCESS) {
        return ret;
    }

    if (attrCatEntryBuffer.rootBlock != -1){
        return SUCCESS;
    }
    
    IndLeaf rootBlockBuf;
    int rootBlock = rootBlockBuf.getBlockNum();
    if (rootBlock == E_DISKFULL)
    {   
        printf("Hi");
        return E_DISKFULL;
    }

    attrCatEntryBuffer.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntryBuffer);

    RelCatEntry recbuffer;
    RelCacheTable::getRelCatEntry(relId, &recbuffer);
    int block = recbuffer.firstBlk;
    while (block != -1) {
        RecBuffer blockBuffer (block);
        unsigned char slotmap[recbuffer.numSlotsPerBlk];
        blockBuffer.getSlotMap(slotmap);

        for (int slot = 0; slot < recbuffer.numSlotsPerBlk; slot++)
        {
            if (slotmap[slot] == SLOT_OCCUPIED)
            {
                Attribute record[recbuffer.numAttrs];
                blockBuffer.getRecord(record, slot);
                RecId recId = RecId{block, slot};
                ret = bPlusInsert(relId, attrName, record[attrCatEntryBuffer.offset], recId);
                if (ret == E_DISKFULL) {

                    printf("Hi1");
                    return E_DISKFULL;
                }
            }
        }
        HeadInfo header;
        blockBuffer.getHeader(&header);
        block = header.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId) {
    AttrCatEntry attrCatEntryBuffer;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntryBuffer);
    if (ret != SUCCESS){
        return ret;
    }
    int rootBlock = attrCatEntryBuffer.rootBlock;
    if (rootBlock == -1){
        return E_NOINDEX;
    }
    int leafBlkNum = findLeafToInsert(rootBlock, attrVal, attrCatEntryBuffer.attrType); 
    Index indexEntry; 
    indexEntry.attrVal = attrVal, indexEntry.block = recId.block, indexEntry.slot = recId.slot;
    if (insertIntoLeaf(relId, attrName, leafBlkNum, indexEntry) == E_DISKFULL)
    {
        
        printf("Hi2");
        BPlusTree::bPlusDestroy(rootBlock);
        attrCatEntryBuffer.rootBlock = -1;
        AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatEntryBuffer);
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType) {
    int blockNum = rootBlock;
    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF) 
    {  
        IndInternal iBlock (blockNum);
        HeadInfo header;
        iBlock.getHeader(&header);
        int i = 0;
        while (i < header.numEntries)
        {
            InternalEntry temp;
            iBlock.getEntry(&temp, i);

            if (compareAttrs(attrVal, temp.attrVal, attrType) <= 0){
                break;
            }
            i++;
        }

        if (i == header.numEntries) 
        {
            InternalEntry temp;
            iBlock.getEntry(&temp, header.numEntries-1);
            blockNum = temp.rChild;
        } 
        else 
        {
            InternalEntry temp;
            iBlock.getEntry(&temp, i);
            blockNum = temp.lChild;
        }
    }
    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry) 
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
    IndLeaf leaf(blockNum);

    HeadInfo header;   
    Index leafEntry;
    leaf.getHeader(&header);
    int index = 0;
    int i = 0;
    Index indices[header.numEntries + 1];
    for (i = 0; i < header.numEntries; i++)
    {
        leaf.getEntry(&leafEntry, i);
        if (compareAttrs(leafEntry.attrVal, indexEntry.attrVal, attrCatBuf.attrType) >= 0)
        {
            break;
        }
        indices[index] = leafEntry;
        index ++;
    }
    indices[index] = indexEntry;
    index ++;
    for (;i < header.numEntries; i++) {
        leaf.getEntry(&leafEntry, i);
        indices[index] = leafEntry;
        index ++;
    }

    if (header.numEntries != MAX_KEYS_LEAF) {
        header.numEntries++;
        leaf.setHeader(&header);
        for (int i = 0; i < header.numEntries; i++){
            leaf.setEntry(&indices[i], i);
        }
        return SUCCESS;
    }
    int newRightBlk = splitLeaf(blockNum, indices);
    if (newRightBlk == E_DISKFULL) {

        printf("Hi3");
        return E_DISKFULL;
    }
    if (header.pblock != -1)
    {
        InternalEntry internalEntry;
        internalEntry.attrVal = indices[MIDDLE_INDEX_LEAF].attrVal, 
        internalEntry.lChild = blockNum;
        internalEntry.rChild = newRightBlk;
        insertIntoInternal(relId, attrName, header.pblock, internalEntry);
    } 
    else 
    {
        createNewRoot(relId, attrName, indices[MIDDLE_INDEX_LEAF].attrVal, blockNum, newRightBlk);
    }
    return SUCCESS;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index indices[]) {
    IndLeaf rightBlk;
    IndLeaf leftBlk(leafBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leafBlockNum;

    if (rightBlkNum == E_DISKFULL) 
    {

        printf("Hi4");
        return E_DISKFULL;
    }
    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);
    
    rightBlkHeader.numEntries = (MAX_KEYS_LEAF+1)/2;
    rightBlkHeader.pblock = leftBlkHeader.pblock, 
    rightBlkHeader.lblock = leftBlkNum, 
    rightBlkHeader.rblock = leftBlkHeader.rblock;
    rightBlk.setHeader(&rightBlkHeader);
    

    leftBlkHeader.numEntries = (MAX_KEYS_LEAF+1)/2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    for (int i = 0; i <= MIDDLE_INDEX_LEAF; i++)
    {
        leftBlk.setEntry(&indices[i], i);
        rightBlk.setEntry(&indices[i + MIDDLE_INDEX_LEAF+1], i);
    }
    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry) {
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
    IndInternal intBlk (intBlockNum);

    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);
    InternalEntry entry[blockHeader.numEntries + 1];
    int i = 0;
    int index = 0;
    InternalEntry iBlock;

    for(i = 0; i < blockHeader.numEntries; i++) {
        intBlk.getEntry(&iBlock, i);
        if (compareAttrs(iBlock.attrVal, intEntry.attrVal, attrCatBuf.attrType) >= 0) {
            if (index > 0){
                entry[index-1].rChild = intEntry.lChild;
            }
            break;
        }
        entry[index++] = iBlock;
    }
    entry[index++] = intEntry;
    if (i < blockHeader.numEntries) {
        intBlk.getEntry(&iBlock, i);
        iBlock.lChild = intEntry.rChild;
        entry[index] = iBlock;
        index ++;
        i++;
    }
    for (; i < blockHeader.numEntries; i++) {
        intBlk.getEntry(&iBlock, i);
        entry[index] = iBlock;
        index ++;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL) {
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);
        for (int j = 0; j < blockHeader.numEntries; j++)
            intBlk.setEntry(&entry[j], j);
        return SUCCESS;
    }

    int newRightBlk = splitInternal(intBlockNum, entry);
    if (newRightBlk == E_DISKFULL)
    {

        printf("Hi5");
        BPlusTree::bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }
    if (blockHeader.pblock != -1)
    {
        InternalEntry iEntry;
        iEntry.lChild = intBlockNum;
        iEntry.rChild = newRightBlk;
        iEntry.attrVal = entry[MIDDLE_INDEX_INTERNAL].attrVal;  
        return insertIntoInternal(relId, attrName, blockHeader.pblock, iEntry);
    } 
    else 
    {
        return createNewRoot(relId, attrName, entry[MIDDLE_INDEX_INTERNAL].attrVal, intBlockNum, newRightBlk);
    }
    return SUCCESS;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[]) 
{
    IndInternal rightBlk;
    IndInternal leftBlock (intBlockNum);
    int leftBlkNum = intBlockNum;
    int rightBlkNum = rightBlk.getBlockNum();
    if (rightBlkNum == E_DISKFULL)
    {
        
        printf("Hi6");
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlock.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);
    
    rightBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;

    rightBlk.setHeader(&rightBlkHeader);
    leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    leftBlkHeader.rblock = rightBlkNum;

    leftBlock.setHeader(&leftBlkHeader);
    for (int entryindex = 0; entryindex < MIDDLE_INDEX_INTERNAL; entryindex++)
    {
        leftBlock.setEntry(&internalEntries[entryindex], entryindex);
        rightBlk.setEntry(&internalEntries[entryindex + MIDDLE_INDEX_INTERNAL + 1], entryindex);
    }
    int type = StaticBuffer::getStaticBlockType(internalEntries[0].lChild);
    BlockBuffer blockbuffer (internalEntries[MIDDLE_INDEX_INTERNAL+1].lChild);

    HeadInfo blockHeader;
    blockbuffer.getHeader(&blockHeader);
    blockHeader.pblock = rightBlkNum;
    blockbuffer.setHeader(&blockHeader);

    for (int i = 0; i < MIDDLE_INDEX_INTERNAL; i++)
    {
        BlockBuffer blockbuffer (internalEntries[i + MIDDLE_INDEX_INTERNAL+1].rChild);
        blockbuffer.getHeader(&blockHeader);
        blockHeader.pblock = rightBlkNum;
        blockbuffer.setHeader(&blockHeader);
    }
    return rightBlkNum; 
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild) {
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum();
    if (newRootBlkNum == E_DISKFULL) 
    {
        BPlusTree::bPlusDestroy(rChild);
        
        printf("Hi7");
        return E_DISKFULL;
    }
    HeadInfo blockHeader;
    newRootBlk.getHeader(&blockHeader);
    blockHeader.numEntries = 1;
    newRootBlk.setHeader(&blockHeader);

    InternalEntry iEntry;
    iEntry.lChild = lChild;
    iEntry.rChild = rChild;
    iEntry.attrVal = attrVal;
    newRootBlk.setEntry(&iEntry, 0);

    BlockBuffer leftChildBlock (lChild);
    BlockBuffer rightChildBlock (rChild);

    HeadInfo leftChildHeader, rightChildHeader;
    leftChildBlock.getHeader(&leftChildHeader);
    rightChildBlock.getHeader(&rightChildHeader);

    leftChildHeader.pblock = newRootBlkNum;
    rightChildHeader.pblock = newRootBlkNum;

    leftChildBlock.setHeader(&leftChildHeader);
    rightChildBlock.setHeader(&rightChildHeader);
    attrCatBuf.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName,  &attrCatBuf);
    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum) {
    if (rootBlockNum < 0 || rootBlockNum >= DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF) 
    {
        IndLeaf leaf (rootBlockNum);
        leaf.releaseBlock();
        return SUCCESS;
    } 
    else if (type == IND_INTERNAL) 
    {
        IndInternal iBlock (rootBlockNum);
        HeadInfo blockHeader;
        iBlock.getHeader(&blockHeader);
        
        InternalEntry blockEntry;
        iBlock.getEntry (&blockEntry, 0);
        BPlusTree::bPlusDestroy(blockEntry.lChild);
        for (int entry = 0; entry < blockHeader.numEntries; entry++) {
            iBlock.getEntry (&blockEntry, entry);
            BPlusTree::bPlusDestroy(blockEntry.rChild);
        }
        iBlock.releaseBlock();
        return SUCCESS;
    } 
    else{ 
        return E_INVALIDBLOCK;
    }
}