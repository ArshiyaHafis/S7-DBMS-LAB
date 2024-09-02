#include "Algebra.h"
#include<iostream>
#include <cstring>

bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}




int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }
    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (relCatEntry.numAttrs != nAttrs)
    {
        return E_NATTRMISMATCH;
    }
    Attribute recordValues[nAttrs];
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);
        int type = attrCatEntry.attrType;
        if (type == NUMBER)
        {
            if (isNumber(record[i]))
            {
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]); 
        }
    }
    return BlockAccess::insert(relId, recordValues);

}



int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    
	int srcRelId = OpenRelTable::getRelId(srcRel);      
    if (srcRelId == E_RELNOTOPEN) 
    {
		return E_RELNOTOPEN;
	}
	AttrCatEntry attrCatEntry;
	int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
	if (ret != SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    int type = attrCatEntry.attrType;
	Attribute attrVal;

	if (type == NUMBER) 
    {
		if (isNumber(strVal)) 
        {
            attrVal.nVal = atof(strVal);
		} 
        else 
        {
			return E_ATTRTYPEMISMATCH;
		}
	} 
    else if (type == STRING) 
    {
		strcpy(attrVal.sVal, strVal);
	}
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    int src_nAttrs = relCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];
    for (int i = 0; i < src_nAttrs; i++) 
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        strcpy(attr_names[i], attrCatEntry.attrName);
        attr_types[i] = attrCatEntry.attrType;
    }

    ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0){
        Schema::deleteRel(targetRel);
        return ret;
    }

    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelId);
    RelCacheTable::resetSearchIndex(targetRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);
    ret = BlockAccess::search(srcRelId, record, attr, attrVal, op);
    while (ret==SUCCESS) 
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) 
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::search(srcRelId, record, attr, attrVal, op);
    }
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
    int srcRelId =  OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int numAttrs = srcRelCatEntry.numAttrs;
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];
    
    for (int i = 0; i < numAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    ret = BlockAccess::project(srcRelId, record);
    while (ret== SUCCESS)
    {
        ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::project(srcRelId, record);
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}



int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int src_nAttrs = srcRelCatEntry.numAttrs;
    
    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];
    for (int i = 0; i < tar_nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
        if (ret != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }

        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types); 
    if (ret != SUCCESS)
    {
        return ret;
    }
    
    int targetRelId = OpenRelTable::openRel(targetRel);
    if(targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];
    ret = BlockAccess::project(srcRelId, record);
    while (ret== SUCCESS) {
        Attribute proj_record[tar_nAttrs];
        for (int i = 0; i < tar_nAttrs; i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }
        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            ret = Schema::deleteRel(targetRel);
            return ret;
        }
        ret = BlockAccess::project(srcRelId, record);
    }

    ret = Schema::closeRel(targetRel);
    if (ret != SUCCESS)
    {
        printf("Invalid Relation ID.\n");
        exit(1);
    }

    return SUCCESS;
}


int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE]) {
    int srcRelId1 = OpenRelTable::getRelId(srcRelation1);
    int srcRelId2 = OpenRelTable::getRelId(srcRelation2);
    if(srcRelId1 == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }
    if(srcRelId2 == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId1, attribute1, &attrCatEntry1);
    if (ret == E_ATTRNOTEXIST){
        printf("%s", attribute1);
        return E_ATTRNOTEXIST;
    }
    ret = AttrCacheTable::getAttrCatEntry(srcRelId2, attribute2, &attrCatEntry2);
    if (ret == E_ATTRNOTEXIST){
        printf("%s", attribute1);
        return E_ATTRNOTEXIST;
    }
    if (attrCatEntry1.attrType != attrCatEntry2.attrType){
        return E_ATTRTYPEMISMATCH;
    }

    RelCatEntry relCatEntry1, relCatEntry2;
    RelCacheTable::getRelCatEntry(srcRelId1, &relCatEntry1);
    RelCacheTable::getRelCatEntry(srcRelId2, &relCatEntry2);
    
    AttrCatEntry temp1, temp2;
    
    for(int j = 0; j < relCatEntry2.numAttrs; j++) {
        if(j == attrCatEntry2.offset)
            continue;

        AttrCacheTable::getAttrCatEntry(srcRelId2, j, &temp2);
        for(int i = 0; i < relCatEntry1.numAttrs; i++) {
            AttrCacheTable::getAttrCatEntry(srcRelId1, i, &temp1);
            if(strcmp(temp1.attrName, temp2.attrName) == 0)
                return E_DUPLICATEATTR;
        }
    }
    int numOfAttributes1 = relCatEntry1.numAttrs;
    int numOfAttributes2 = relCatEntry2.numAttrs;

    if(attrCatEntry2.rootBlock == -1) {
        ret = BPlusTree::bPlusCreate(srcRelId2, attrCatEntry2.attrName);
        if(ret != SUCCESS){
            return ret;
        }
    }

    int numOfAttributesInTarget = numOfAttributes1 + numOfAttributes2 - 1;
    char targetRelAttrNames[numOfAttributesInTarget][ATTR_SIZE];
    int targetRelAttrTypes[numOfAttributesInTarget];
    
    for(int i = 0; i < relCatEntry1.numAttrs; i++) {
        AttrCacheTable::getAttrCatEntry(srcRelId1, i, &temp1);
        strcpy(targetRelAttrNames[i], temp1.attrName);
        targetRelAttrTypes[i] = temp1.attrType;
    }
    for(int j = 0; j < attrCatEntry2.offset; j++) {
        AttrCacheTable::getAttrCatEntry(srcRelId2, j, &temp2);
        strcpy(targetRelAttrNames[numOfAttributes1 + j], temp2.attrName);
        targetRelAttrTypes[numOfAttributes1 + j] = temp2.attrType;
    }
    for(int j = attrCatEntry2.offset + 1; j < relCatEntry2.numAttrs; j++) {
        AttrCacheTable::getAttrCatEntry(srcRelId2, j, &temp2);
        strcpy(targetRelAttrNames[numOfAttributes1 + j - 1], temp2.attrName);
        targetRelAttrTypes[numOfAttributes1 + j - 1] = temp2.attrType;
    }

    ret = Schema::createRel(targetRelation, numOfAttributesInTarget, targetRelAttrNames, targetRelAttrTypes);
    if(ret != SUCCESS){
        return ret;
    }
    int targetRelId = OpenRelTable::openRel(targetRelation);
    if(targetRelId == E_RELNOTOPEN)
    {
        Schema::deleteRel(targetRelation);
        return E_RELNOTOPEN;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[numOfAttributesInTarget];

    RelCacheTable::resetSearchIndex(srcRelId1);
    while (BlockAccess::project(srcRelId1, record1) == SUCCESS) {
        RelCacheTable::resetSearchIndex(srcRelId2);
        AttrCacheTable::resetSearchIndex(srcRelId2, attribute2);
        while (BlockAccess::search(
            srcRelId2, record2, attribute2, record1[attrCatEntry1.offset], EQ
        ) == SUCCESS ) {
            for(int i = 0; i < numOfAttributes1; i++)
                targetRecord[i] = record1[i];
            for(int j = 0; j < attrCatEntry2.offset; j++)
                targetRecord[numOfAttributes1 + j] = record2[j];
            for(int j = attrCatEntry2.offset + 1; j < numOfAttributes2; j++)
                targetRecord[numOfAttributes1 + j - 1] = record2[j];
            ret = BlockAccess::insert(targetRelId, targetRecord);

            if(ret != SUCCESS) {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }
    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}