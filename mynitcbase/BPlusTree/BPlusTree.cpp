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