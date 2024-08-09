#include "RelCacheTable.h"
#include <cstring>
#include <iostream>
RelCacheEntry* RelCacheTable::relCache[MAX_OPEN];

/*
Get the relation catalog entry for the relation with rel-id `relId` from the cache
NOTE: this function expects the caller to allocate memory for `*relCatBuf`
*/
int RelCacheTable::getRelCatEntry(int relId, RelCatEntry* relCatBuf) 
{
    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    // if there's no entry at the rel-id
    if (relCache[relId] == nullptr) {
        return E_RELNOTOPEN;
    }

    // copy the value to the relCatBuf argument
    *relCatBuf = relCache[relId]->relCatEntry;
    return SUCCESS;
}

/* Converts a relation catalog record to RelCatEntry struct
    We get the record as Attribute[] from the BlockBuffer.getRecord() function.
    This function will convert that to a struct RelCatEntry type.
NOTE: this function expects the caller to allocate memory for `*relCatEntry`
*/
void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) 
{
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
    /* fill the rest of the relCatEntry struct with the values at
        RELCAT_NO_RECORDS_INDEX,
        RELCAT_FIRST_BLOCK_INDEX,
        RELCAT_LAST_BLOCK_INDEX,
        RELCAT_NO_SLOTS_PER_BLOCK_INDEX
    */
}


/* will return the searchIndex for the relation corresponding to `relId
NOTE: this function expects the caller to allocate memory for `*searchIndex`
*/
int RelCacheTable::getSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr) {
        return E_RELNOTOPEN;
    }
    *searchIndex = relCache[relId]->searchIndex;
    return SUCCESS;
}

// sets the searchIndex for the relation corresponding to relId
int RelCacheTable::setSearchIndex(int relId, RecId* searchIndex) {
    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr) {
        return E_RELNOTOPEN;
    }
    relCache[relId]->searchIndex = *searchIndex;
    return SUCCESS;
  // update the searchIndex value in the relCache for the relId to the searchIndex argument

  
}

int RelCacheTable::resetSearchIndex(int relId) {
    RecId searchIndex = {-1, -1};
    return setSearchIndex(relId, &searchIndex);
}


int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf)
{
    if (relId < 0 || relId >= MAX_OPEN)
    {
        return E_OUTOFBOUND;
    }
    if (relCache[relId] == nullptr)
    {
        return E_RELNOTOPEN;
    }
    relCache[relId]->relCatEntry = *relCatBuf;
    relCache[relId]->dirty = true;

    return SUCCESS;
}



void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry, union Attribute record[RELCAT_NO_ATTRS])
{

    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, relCatEntry->relName);
    record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
    record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
    record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
    record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
    record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
}