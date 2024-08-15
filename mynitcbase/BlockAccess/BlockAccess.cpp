#include "BlockAccess.h"

#include <cstring>
#include <iostream>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);
    int block = -1;
    int slot = -1;

    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);

    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        

        block = relCatBuf.firstBlk;
        slot = 0;
    }
    else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }
    while (block != -1) {
        RecBuffer recBuffer(block);
        Attribute recordEntry[relCatBuf.numAttrs];
        HeadInfo header;

        recBuffer.getRecord(recordEntry, slot);
        recBuffer.getHeader(&header);
        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);


        if (slot >= header.numSlots) {
            block = header.rblock;
            slot = 0;
            continue;
        }

        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

        AttrCatEntry attrCatBuf;
        int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);
        Attribute currRecordAttr = recordEntry[attrCatBuf.offset];

        int cmpVal = compareAttrs(currRecordAttr, attrVal, attrCatBuf.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) 
        {
            RecId searchIndex = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);
            return searchIndex;
        }
        
        slot++;
    }

    return RecId({-1, -1});
}


int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    memcpy(newRelationName.sVal, newName, ATTR_SIZE);
    char relcatAttrName[]=RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, newRelationName, EQ);

    if(recId.block != -1 && recId.slot != -1)
    {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    memcpy(oldRelationName.sVal, oldName, ATTR_SIZE);
    recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, oldRelationName, EQ);
    if(recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    RecBuffer recBuffer(recId.block);
    Attribute recordEntry[RELCAT_NO_ATTRS];
    recBuffer.getRecord(recordEntry, recId.slot);
    memcpy(&recordEntry[RELCAT_REL_NAME_INDEX], &newRelationName, ATTR_SIZE);

    recBuffer.setRecord(recordEntry, recId.slot);
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributes = recordEntry[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    char attrcatAttrName[] = ATTRCAT_ATTR_RELNAME;
    for(int i=0; i<numberOfAttributes; i++){
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, oldRelationName, EQ);
        if(recId.block == -1 && recId.slot == -1){
            return E_RELNOTEXIST;
        }
        RecBuffer recBuffer(recId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        recBuffer.getRecord(attrCatRecord, recId.slot);
        memcpy(&attrCatRecord[ATTRCAT_REL_NAME_INDEX], &newRelationName, ATTR_SIZE);
        recBuffer.setRecord(recordEntry, recId.slot);
    }

    return SUCCESS;
}



int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    memcpy(relNameAttr.sVal, relName, ATTR_SIZE);
    char relcatAttrName[]=RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, relNameAttr, EQ);    
    if(recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }


    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    RecId attrToRenameRecId{-1, -1};
    char attrcatAttrName[]=ATTRCAT_ATTR_RELNAME;
    while (true) {
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, relNameAttr, EQ);
        if(recId.block == -1 && recId.slot == -1){
            break;
        }
        RecBuffer recBuffer(recId.block);
        Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
        recBuffer.getRecord(attrCatEntryRecord, recId.slot);
        char attrName[ATTR_SIZE];
        memcpy(attrName, attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, ATTR_SIZE);

        if(strcmp(attrName, oldName) == 0){
            attrToRenameRecId = recId;
            break;
        }
        if(strcmp(attrName, newName) == 0){
            return E_ATTREXIST;
        }
    }

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1){
        return E_ATTRNOTEXIST;
    }

    RecBuffer recBuffer(attrToRenameRecId.block);
    Attribute recAttr[ATTRCAT_NO_ATTRS];
    recBuffer.getRecord(recAttr, attrToRenameRecId.slot);
    memcpy(recAttr[ATTRCAT_ATTR_NAME_INDEX].sVal, newName, ATTR_SIZE);
    recBuffer.setRecord(recAttr, attrToRenameRecId.slot);
    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record)
{
    RelCatEntry relCatEntry;
    int ret = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int blockNum = relCatEntry.firstBlk;
    RecId recId = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk; 
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;
    while (blockNum != -1)
    {
        RecBuffer recBuffer(blockNum);
        struct HeadInfo head;
        recBuffer.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);
        for (int i = 0; i < head.numSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                recId.block = blockNum;
                recId.slot = i;
                break;
            }
        }
        
        if (recId.block != -1 && recId.slot != -1)
        {
            break;
        }
        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }
    if (recId.block == -1 && recId.slot == -1)
    {
        if (relId == RELCAT_RELID)
        {
            return E_MAXRELATIONS;
        }
        RecBuffer recBuffer;
        int newBlockNum = recBuffer.getBlockNum();
        if (newBlockNum == E_DISKFULL)
        {
            return E_DISKFULL;
        }
        recId.block = newBlockNum;
        recId.slot = 0;

        HeadInfo head;
        recBuffer.getHeader(&head);
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = prevBlockNum;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;
        recBuffer.setHeader(&head);
        
        unsigned char slotMap[head.numSlots];
        for (int i = 0; i < head.numSlots; i++)
        {
            slotMap[i] = SLOT_UNOCCUPIED;
        }

        recBuffer.setSlotMap(slotMap);


        if (prevBlockNum != -1)
        {
            RecBuffer recBuffer(prevBlockNum);
            HeadInfo prevBlockHeader;
            recBuffer.getHeader(&prevBlockHeader);
            prevBlockHeader.rblock = recId.block;
            recBuffer.setHeader(&prevBlockHeader);
        }
        else
        {
            relCatEntry.firstBlk = recId.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }
        relCatEntry.lastBlk = recId.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }
    RecBuffer recBuffer(recId.block);
    recBuffer.setRecord(record, recId.slot);

    HeadInfo head;
    recBuffer.getHeader(&head);
    unsigned char slotMap[head.numSlots];
    recBuffer.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);
    head.numEntries++;
    recBuffer.setHeader(&head);
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}


int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    RecId recId;
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    
    if(ret != SUCCESS){
        return ret;
    }

    int rootBlock = attrCatEntry.rootBlock;
    if(rootBlock == -1) 
    {
        recId = linearSearch(relId, attrName, attrVal, op);
    } 
    else 
    {
        recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
    }

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }
    
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    
    const char relcatAttrName[] = RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *) relcatAttrName, relNameAttr, EQ);

    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer recBuffer(recId.block);
    recBuffer.getRecord(relCatEntryRecord, recId.slot);
    
    int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    int recBlockNum = firstBlock;
    while (recBlockNum != -1)
    {

        RecBuffer recBuffer(recBlockNum);
        HeadInfo header;
        recBuffer.getHeader(&header);
        recBlockNum = header.rblock;
        recBuffer.releaseBlock();
    }

    int numberOfAttributesDeleted = 0;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    while (true)
    {
        RecId attrCatRecId;
        const char constRelNameAttr[] = ATTRCAT_ATTR_RELNAME;
        attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *) constRelNameAttr, relNameAttr, EQ);
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }

        numberOfAttributesDeleted++;
        RecBuffer recBuffer(attrCatRecId.block);
        HeadInfo header;
        recBuffer.getHeader(&header);
        Attribute recordEntry[ATTRCAT_NO_ATTRS];
        recBuffer.getRecord(recordEntry, attrCatRecId.slot);
        
        int rootBlock = recordEntry[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        unsigned char slotMap[header.numSlots];

        recBuffer.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        recBuffer.setSlotMap(slotMap);


        header.numEntries--;
        recBuffer.setHeader(&header);
        if (header.numEntries==0)
        {
            HeadInfo leftBlockHeader;
            RecBuffer leftBlockRecBuffer(header.lblock);
            leftBlockRecBuffer.getHeader(&leftBlockHeader);
            if (header.rblock != -1)
            {
                HeadInfo rightBlockHeader;
                RecBuffer rightBlockRecBuffer(header.rblock);
                rightBlockRecBuffer.getHeader(&rightBlockHeader);
            }
            else
            {
                RelCatEntry relCatEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                relCatEntry.lastBlk = header.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
            }
            recBuffer.releaseBlock();
        }
    }

    RecBuffer relCatRecBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    relCatRecBuffer.getHeader(&relCatHeader);
    
    relCatHeader.numEntries--;
    unsigned char relCatSlotMap[relCatHeader.numSlots];
    relCatRecBuffer.getSlotMap(relCatSlotMap);
    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatRecBuffer.setSlotMap(relCatSlotMap);


    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
    relCatEntry.numRecs -= numberOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
    return SUCCESS;
}



int BlockAccess::project(int relId, Attribute *record) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);
    int block=prevRecId.block, slot=prevRecId.slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1)
    {
        RecBuffer recBuffer(block);
        struct HeadInfo head;
        recBuffer.getHeader(&head);
        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);
        if(slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED){
            slot++;
        }
        else {
            break;
        }
    }

    if (block == -1){
        return E_NOTFOUND;
    }
    RecId nextRecId={block, slot};
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer recBuffer(nextRecId.block);
    recBuffer.getRecord(record, nextRecId.slot);

    return SUCCESS;
}