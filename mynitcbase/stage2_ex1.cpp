#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[])
{
	Disk disk_run;

	// create objects for the relation catalog and attribute catalog
	RecBuffer relCatBuffer(RELCAT_BLOCK);


	HeadInfo relCatHeader;

	// load the headers of both the blocks into relCatHeader and attrCatHeader.
	// (we will implement these functions later)
	relCatBuffer.getHeader(&relCatHeader);

	int relationCount = relCatHeader.numEntries;

	for (int i = 0; i < relationCount; i++)
	{ 

		Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog
		relCatBuffer.getRecord(relCatRecord, i);

		printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

		int attrCatBlockNo = ATTRCAT_BLOCK;
		while(attrCatBlockNo != -1)
		{ 

			RecBuffer attrCatBuffer(attrCatBlockNo);
			HeadInfo attrCatHeader;
			attrCatBuffer.getHeader(&attrCatHeader);
			int attributeCount = attrCatHeader.numEntries;

			for (int j = 0; j < attributeCount; j++){
				Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
				attrCatBuffer.getRecord(attrCatRecord, j);

				if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0)
				{ /* attribute catalog entry corresponds to the current relation */
					const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
					printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
				}
			}

			attrCatBlockNo = attrCatHeader.rblock;
		}
		printf("\n");
	}

	return 0;
}