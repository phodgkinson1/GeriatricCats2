#include "directoryEntry.h"
#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"
#include "freespace.h"
#include "mfsHelper.h"

int rootGlobal;

// Initialize Directory
// **** THIS SHOULD BE CHEKCED.
// Does not work unless extent *allocateBlock() function is implemented and extent table works
// Also de * --> DE array in pointer type *****
int initDir(int defaultEntries, struct DE * parent, int parentIndex)
{
	printf("InitDir() call, passed parentIndex: %d\n", parentIndex);

        struct DE *p = parent;  // Declare p here to make it accessible in the if-else blocks
        printf("p filename: %s\n", p->fileName);

	printf("size Directory entry: %ld\n", sizeof(struct DE));
	int bytesNeeded = defaultEntries * sizeof(DE);
	int blocksNeeded = ((bytesNeeded + BLOCK_SIZE -1) / BLOCK_SIZE);
	bytesNeeded = blocksNeeded * BLOCK_SIZE;
	int actualDirEntries = (bytesNeeded / sizeof(DE));
	printf("byteNeeded: %d, blocksNeeded: %d, actualDirEntries: %d\n", bytesNeeded, blocksNeeded, actualDirEntries);

	printf("call allocation blocks inside initdir()\n");
	EXTENT *tempArray = allocateBlocks(blocksNeeded, blocksNeeded);
	int dirStart= tempArray[0].start;
	int dirExtentBlock= initExtent(actualDirEntries, dirStart);
	printf("In initDir tempArray returned of extent *. length tempArray: %ld, value of tempArray[0] start: %d count: %d\n", 
	sizeof(tempArray) / sizeof(tempArray[0]), tempArray[0].start, tempArray[0].count);

	if(!tempArray || tempArray==NULL || dirExtentBlock == -1)
		{
		printf("No space for directory.\n");
		return -1;
		}

        struct DE *dir = malloc(bytesNeeded);

	//initiate empty entries 2 to n 
        for (int i = 2; i < actualDirEntries; i++)
                {
                // Initialize each entry to unused
                dir[i].fileName[0]= '\0'; // Null-terminated name
		dir[i].fileSize = 0;
    		dir[i].extentBlockStart= dirExtentBlock;
        	dir[i].extentIndex=i;
                time_t t = time(NULL);
                dir[i].createdTime = t;
                dir[i].modifiedTime = t;
                dir[i].lastAccessedTime = t;
                dir[i].isDirectory = 0;

//                printf("filename: %c\n", dir[i].fileName[0]);
                }

	//initiate entyr 0 and 1
        if (p == NULL)
        {
                // Initialize  . and .. manually

        	// If there's no parent (root directory), set p to the root directory entry
        	p = &dir[0];

        	// Initialize "." entry
        	strcpy(p->fileName, ".");
        	p->fileSize = bytesNeeded;
   		p->extentBlockStart = dirExtentBlock;
                p->extentIndex=0;
        	time_t t = time(NULL);
        	p->createdTime = t;
        	p->modifiedTime = t;
        	p->lastAccessedTime = t;
        	p->isDirectory = 1;

	        printf("dir[0].extentStart: %d\n", dir[0].extentBlockStart);
		printf("dir[0].fileName: %s\n", dir[0].fileName);

        	// Initialize ".." entry for root
        	strcpy(dir[1].fileName, "..");
        	dir[1].fileSize = p->fileSize;
       		dir[1].extentBlockStart= dirExtentBlock;
                dir[1].extentIndex=1;
		dir[1].createdTime = p->createdTime;
        	dir[1].modifiedTime = p->modifiedTime;
        	dir[1].lastAccessedTime = p->lastAccessedTime;
        	dir[1].isDirectory = p->isDirectory;

                printf("dir[1].extentStart: %d\n", dir[1].extentBlockStart);
		printf("dir[1].fileName: %s\n", dir[1].fileName);

		rootGlobal = dirStart;
      		//assign global
        	rootDir=dir;
		if(cwd==NULL) cwd=rootDir;
		cwdAbsolutePath= malloc(2);
		//strcpy(cwdAbsolutePath, "/");
   		cwdAbsolutePath[0]='/';
		cwdAbsolutePath[1]='\0';
		}
    	else
	    	{
//		printf("entered else loop\n");

                // Initialize "." entry
                strcpy(dir[0].fileName, ".");
		dir[0].fileSize= (p[1].fileSize);
   		dir[0].extentBlockStart= p[1]. extentBlockStart;
                dir[0].extentIndex=0;
		dir[0].createdTime = p[1].createdTime;
                dir[0].modifiedTime = p[1].modifiedTime;
                dir[0].lastAccessedTime = p[1].lastAccessedTime;
                dir[0].isDirectory = p[1].isDirectory;
	        printf("dir[0].extentStart: %d\n", dir[0].extentBlockStart);


              printf("check subdir[0].fileName: %s\n", dir[0].fileName);
                // Initialize ".." entry for root AS PARENT PASSED
                strcpy(dir[1].fileName, "..");
                dir[1].fileSize = bytesNeeded;
 		dir[1].extentBlockStart= dirExtentBlock;
                dir[1].extentIndex=1;
       		time_t t = time(NULL);
                dir[1].createdTime = t;
                dir[1].modifiedTime = t;
                dir[1].lastAccessedTime = t;
                dir[1].isDirectory = 1;
                printf("initDir- dir[1].extentBlockStart: %d\n", dir[1].extentBlockStart);

		//update parent's entry at index with same information
                p[parentIndex].fileSize = bytesNeeded;
                p[parentIndex].extentBlockStart= dirExtentBlock;
                p[parentIndex].extentIndex =parentIndex;
                p[parentIndex].createdTime = t;
                p[parentIndex].modifiedTime = t;
                p[parentIndex].lastAccessedTime = t;
                p[parentIndex].isDirectory = 1;
                printf("initDir- parent[parentIndex=2].extentBlockStart: %d\n", p[parentIndex].extentBlockStart);
		}


        // Now write it to disk
  	if(LBAwrite(dir, blocksNeeded, dirStart) != blocksNeeded)
		{
            	printf("LBA Write error!\n");
		exit(1);
		}

        if( (dir != NULL) && (rootDir != dir) && (cwd!=dir) )
		{
		free(dir);
		dir = NULL;
		}
	if (tempArray !=NULL)
		{
		free(tempArray);
		tempArray=NULL;
		}
	if(p != NULL) p=NULL;

	//return start of directory location
	return dirStart;
}
