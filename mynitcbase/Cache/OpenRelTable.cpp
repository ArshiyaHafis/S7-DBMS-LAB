#include "OpenRelTable.h"
#include <iostream>
#include <cstring>



OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable()
{
	for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }
    /************ Setting up Relation Cache entries ************/
    /**** setting up Relation Catalog relation in the Relation Cache Table****/
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    struct RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;


    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

	/******************** setting up student relation in the Relation Cache Table ******************/
	relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT+1);

	RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
	relCacheEntry.recId.block = RELCAT_BLOCK;
	relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT+1;

	RelCacheTable::relCache[ATTRCAT_RELID+1] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
	*(RelCacheTable::relCache[ATTRCAT_RELID+1]) = relCacheEntry;
	

	/************ Setting up Attribute cache entries ************/
    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/

	RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrCacheHead = nullptr;
    AttrCacheEntry *temp;

    for (int cache = 0; cache < RELCAT_NO_ATTRS; cache++)
    {
        AttrCacheEntry* newEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attrCatBlock.getRecord(attrCatRecord, cache);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = cache;
        newEntry->next = nullptr;

        if (attrCacheHead == nullptr)
        {
            attrCacheHead = newEntry;
            temp = attrCacheHead;
        }
        else
        {
            temp->next = newEntry;
            temp = newEntry;
        } 
    }
    
    AttrCacheTable::attrCache[RELCAT_RELID] = attrCacheHead;

	/**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    attrCacheHead = nullptr;
    for (int cache = 6; cache < RELCAT_NO_ATTRS + 6; cache++)
    {
        AttrCacheEntry* newEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attrCatBlock.getRecord(attrCatRecord, cache);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = cache;
        newEntry->next = nullptr;

        if (attrCacheHead == nullptr)
        {
            attrCacheHead = newEntry;
            temp = attrCacheHead;
        }
        else
        {
            temp->next = newEntry;
            temp = newEntry;
        }  
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = attrCacheHead;


	/**** setting up student relation in the Attribute Cache Table ****/
	attrCacheHead = nullptr;
	for (int cache = 12; cache < 18; cache++)
    {
        AttrCacheEntry* newEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attrCatBlock.getRecord(attrCatRecord, cache);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &newEntry->attrCatEntry);
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = cache;
        newEntry->next = nullptr;

        if (attrCacheHead == nullptr)
        {
            attrCacheHead = newEntry;
            temp = attrCacheHead;
        }
        else
        {
            temp->next = newEntry;
            temp = newEntry;
        }  
    }
	AttrCacheTable::attrCache[ATTRCAT_RELID+1] = attrCacheHead; 
    tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);

    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
    // set rest as free
    for(int i=2;i<MAX_OPEN;i++){
        tableMetaInfo[i].free=true;
    }

}

OpenRelTable::~OpenRelTable()
{
    for (int i = 2; i < MAX_OPEN; ++i)
    {
        if (!tableMetaInfo[i].free)
        {
        OpenRelTable::closeRel(i); // we will implement this function later
        }
    }

    free(RelCacheTable::relCache[0]);
    free(RelCacheTable::relCache[1]);
}





int OpenRelTable::closeRel(int relId) {
    if (relId==RELCAT_RELID || relId==ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }
    RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
    AttrCacheEntry * attrCacheEntry = AttrCacheTable::attrCache[relId];
    AttrCacheEntry *tempAttrCacheEntry = attrCacheEntry;
    // free all the linked list pointers
    while (tempAttrCacheEntry != nullptr)
    {
        attrCacheEntry = attrCacheEntry->next;
        free(tempAttrCacheEntry);
        tempAttrCacheEntry = attrCacheEntry;
    }
    
    free(relCacheEntry);
    // update `tableMetaInfo` to set `relId` as a free slot
    tableMetaInfo[relId].free = true;
    // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;
}



int OpenRelTable::getFreeOpenRelTableEntry()
{

    for (int i = 2; i < MAX_OPEN; ++i)
    {
        if (tableMetaInfo[i].free)
        {
        return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

    for (int i = 0; i < MAX_OPEN; i++)
    {
        if (strcmp(tableMetaInfo[i].relName, relName) == 0)
        {
        return i;
        }
    }

    return E_RELNOTOPEN;
}


int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
    // let relId be used to store the  slot.
    int relId = getRelId(relName);
    /* the relation `relName` already has an entry in the Open Relation Table */
    if (relId != E_RELNOTOPEN)
    {
        // (checked using OpenRelTable::getRelId())
        // return that relation id;
        return relId;
    }

    /* find a free slot in the Open Relation Table
        using OpenRelTable::getFreeOpenRelTableEntry(). */
    relId = getFreeOpenRelTableEntry();

    if (relId == E_CACHEFULL)
    { /* free slot not available */
        return E_CACHEFULL;
    }

    /****** Setting up Relation Cache entry for the relation ******/
    RecBuffer relCatBlock(RELCAT_BLOCK);

    Attribute relCatRecord[RELCAT_NO_ATTRS];

    // get the record from the relation catalog for the relation relation name is equal to relName using linear search
    char relNameAttrConst[] = RELCAT_ATTR_RELNAME;
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    // reset search index to -1
    RecId recId ={-1,-1};
    RelCacheTable::setSearchIndex(RELCAT_RELID, &recId);
    recId = BlockAccess::linearSearch(RELCAT_RELID, relNameAttrConst,relNameAttr, EQ);

    // if the record is not found, return E_RELNOTEXIST
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }
    // initialise BlockNumber with recId.block
    RecBuffer relCatBuffer(recId.block);
    relCatBuffer.getRecord(relCatRecord, recId.slot);

    struct RelCacheEntry * relCacheEntry=(struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
    relCacheEntry->recId.block = recId.block;
    relCacheEntry->recId.slot = recId.slot;
    relCacheEntry->dirty = false;
    relCacheEntry->searchIndex = recId;


    // allocate this on the heap because we want it to persist outside this function
    RelCacheTable::relCache[relId] = relCacheEntry;
    int numOfAttrs = relCacheEntry->relCatEntry.numAttrs;



    /************ Setting up Attribute cache entries ************/
    // (we need to populate attribute cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *attrLinkedListHead = nullptr;
    AttrCacheEntry *currAttrCacheEntry;

    recId ={-1,-1};
    RelCacheTable::setSearchIndex(ATTRCAT_RELID, &recId);
    recId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameAttrConst,relNameAttr, EQ);

    for(int i=0;i<numOfAttrs&& recId.block!=-1 && recId.slot!=-1;i++){
        if (attrLinkedListHead == nullptr)
        {
        attrLinkedListHead = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        currAttrCacheEntry = attrLinkedListHead;
        }
        else
        {
        currAttrCacheEntry->next = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        currAttrCacheEntry = currAttrCacheEntry->next;
        }
        RecBuffer attrCatBlock(recId.block);
        attrCatBlock.getRecord(attrCatRecord, recId.slot);
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &currAttrCacheEntry->attrCatEntry);
        currAttrCacheEntry->recId.block = recId.block;
        currAttrCacheEntry->recId.slot = recId.slot;
        currAttrCacheEntry->dirty = false;
        // currAttrCacheEntry->searchIndex = indexId;

        recId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameAttrConst,relNameAttr, EQ);



    } 


    AttrCacheTable::attrCache[relId] = attrLinkedListHead; // head of the linked list




    
    // update the relIdth entry of the tableMetaInfo with free as false and
    // relName as the input.
    tableMetaInfo[relId].free = false;
    strcpy(tableMetaInfo[relId].relName, relName);

    return relId;
}