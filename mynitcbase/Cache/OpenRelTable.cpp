#include "OpenRelTable.h"
#include <iostream>
#include <cstring>


int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    if (strcmp(relName, RELCAT_RELNAME) == 0)
        return RELCAT_RELID;
    if (strcmp(relName, ATTRCAT_RELNAME) == 0)
        return ATTRCAT_RELID;

    return E_RELNOTOPEN;
}

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

}

OpenRelTable::~OpenRelTable()
{
  // free all the memory that you allocated in the constructor
}