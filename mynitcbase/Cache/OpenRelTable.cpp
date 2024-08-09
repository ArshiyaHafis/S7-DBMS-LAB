#include "OpenRelTable.h"
#include <iostream>
#include <cstring>



OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

AttrCacheEntry* createList(int length) {
    AttrCacheEntry* head = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
    AttrCacheEntry* tail = head;
    for (int i = 1; i < length; i++) {
        tail->next = (AttrCacheEntry*) malloc(sizeof(AttrCacheEntry));
        tail = tail->next;
    }
    tail->next = nullptr;
    return head;
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
    tableMetaInfo[RELCAT_RELID].free = false;
    memcpy(tableMetaInfo[RELCAT_RELID].relName, relCacheEntry.relCatEntry.relName, ATTR_SIZE);


    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;
    tableMetaInfo[RELCAT_RELID].free = false;
    memcpy(tableMetaInfo[RELCAT_RELID].relName, relCacheEntry.relCatEntry.relName, ATTR_SIZE);

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
        OpenRelTable::closeRel(i);
        }
    }
    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty)
    {
        RelCatEntry relCacheEntry = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
        union Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCacheEntry, record);
        RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;

        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record, recId.slot);
    }

    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    if (RelCacheTable::relCache[RELCAT_RELID]->dirty)
    {
        RelCatEntry relCacheEntry = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
        union Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&relCacheEntry, record);
        RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
        RecBuffer relCatBlock(recId.block);
        relCatBlock.setRecord(record, recId.slot);
    }
    free(RelCacheTable::relCache[RELCAT_RELID]);
    for (AttrCacheEntry* temp = AttrCacheTable::attrCache[RELCAT_RELID], *next; temp != nullptr; temp = next) {
        next = temp->next;
        free(temp);
    }
    for (AttrCacheEntry* temp = AttrCacheTable::attrCache[ATTRCAT_RELID], *next; temp != nullptr; temp = next) {
        next = temp->next;
        free(temp);
    }
}


void clearList(AttrCacheEntry* head) {
    for (AttrCacheEntry* it = head, *next; it != nullptr; it = next) {
        next = it->next;
        free(it);
    }
}


int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;

    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if (OpenRelTable::tableMetaInfo[relId].free)
        return E_RELNOTOPEN;

    if (RelCacheTable::relCache[relId]->dirty) {
        Attribute record[RELCAT_NO_ATTRS];

        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[relId]->relCatEntry, record);

        RecBuffer relCatBuffer(RelCacheTable::relCache[relId]->recId.block);

        relCatBuffer.setRecord(record, RelCacheTable::relCache[relId]->recId.slot);
    }

    OpenRelTable::tableMetaInfo[relId].free = true;
    free(RelCacheTable::relCache[relId]);
    clearList(AttrCacheTable::attrCache[relId]);

    RelCacheTable::relCache[relId] = nullptr;
    AttrCacheTable::attrCache[relId] = nullptr;

    return SUCCESS;


}
// int OpenRelTable::closeRel(int relId) {
//     if (relId==RELCAT_RELID || relId==ATTRCAT_RELID) {
//         return E_NOTPERMITTED;
//     }

//     if (relId < 0 || relId >= MAX_OPEN) {
//         return E_OUTOFBOUND;
//     }

//     if (tableMetaInfo[relId].free) {
//         return E_RELNOTOPEN;
//     }

//     if (RelCacheTable::relCache[relId]->dirty) {
//         RelCatEntry relCatEntry = RelCacheTable::relCache[relId]->relCatEntry;
//         Attribute record[RELCAT_NO_ATTRS];
//         RelCacheTable::relCatEntryToRecord(&relCatEntry, record);
//         RecId recId = RelCacheTable::relCache[relId]->recId;
//         RecBuffer relCatBlock(recId.block);
//         relCatBlock.setRecord(record, recId.slot);
//     }

//     AttrCacheEntry* head = AttrCacheTable::attrCache[relId];
//     for (AttrCacheEntry* temp = head, *next; temp != nullptr; temp = next) {
//         next = temp->next;
//         free(temp);
//     }
//     OpenRelTable::tableMetaInfo[relId].free = true;
//     free(RelCacheTable::relCache[relId]);

//     RelCacheTable::relCache[relId] = nullptr;
//     AttrCacheTable::attrCache[relId] = nullptr;

//     return SUCCESS;
// }



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
        if (strcmp(tableMetaInfo[i].relName, relName) == 0 && !tableMetaInfo[i].free )
        {
            return i;
        }
    }

    return E_RELNOTOPEN;
}



int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
    int relId = OpenRelTable::getRelId(relName);
    if (relId != E_RELNOTOPEN)
    {
        return relId;
    }

    relId = OpenRelTable::getFreeOpenRelTableEntry();

    if (relId == E_CACHEFULL)
    {
        return E_CACHEFULL;
    }

    /****** Setting up Relation Cache entry for the relation ******/
    RecBuffer relCatBlock(RELCAT_BLOCK);
    char relNameAttrConst[] = RELCAT_ATTR_RELNAME;
    Attribute relNameAttr;
    memcpy(relNameAttr.sVal, relName, ATTR_SIZE);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relNameAttrConst,relNameAttr, EQ);

    // if the record is not found, return E_RELNOTEXIST
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }
    // initialise BlockNumber with recId.block
    RecBuffer relCatBuffer(recId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, recId.slot);

    RelCatEntry relCatEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCatEntry);

    RelCacheTable::relCache[relId] = (RelCacheEntry*) malloc(sizeof(RelCacheEntry));

    RelCacheTable::relCache[relId]->recId = recId;
    RelCacheTable::relCache[relId]->relCatEntry = relCatEntry;

    /****** Setting up Attribute Cache entry for the relation ******/
    int numAttrs = relCatEntry.numAttrs;
    AttrCacheEntry* listHead = createList(numAttrs);
    AttrCacheEntry* node = listHead;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    while(true) {
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        if (attrcatRecId.block != -1 && attrcatRecId.slot != -1) {
            Attribute attrcatRecord[ATTRCAT_NO_ATTRS];

            RecBuffer attrRecBuffer(attrcatRecId.block);

            attrRecBuffer.getRecord(attrcatRecord, attrcatRecId.slot);

            AttrCatEntry attrCatEntry;
            AttrCacheTable::recordToAttrCatEntry(attrcatRecord, &attrCatEntry);

            node->recId = attrcatRecId;
            node->attrCatEntry = attrCatEntry;
            node = node->next;
        }
        else 
            break;
    }

    AttrCacheTable::attrCache[relId] = listHead;
    /****** Setting up metadata in the Open Relation Table for the relation******/

    OpenRelTable::tableMetaInfo[relId].free = false;
    memcpy(OpenRelTable::tableMetaInfo[relId].relName, relCatEntry.relName, ATTR_SIZE);

    return relId;
}