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

