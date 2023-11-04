#include "directoryEntry.h"
#include "fsLow.h"
#include "mfs.h"
#include "vcb.h"
#include "freespace.h"
#include "mfsHelper.h"

// Initialize Directory
// **** THIS SHOULD BE CHEKCED.
// Does not work unless extent *allocateBlock() function is implemented and extent table works
// Also de * --> DE array in pointer type *****
int initDir(int defaultEntries, struct DE * parent)
{
        struct DE *p = parent;  // Declare p here to make it accessible in the if-else blocks
//        printf("p filename: %s\n", p->fileName);

//	printf("size Directory entry: %ld\n", sizeof(struct DE));
	//fill parent given (".") with location returned from allocateBlocks- should be block 7
//	printf("\nINITDIR CALLED\n");
	int bytesNeeded = defaultEntries * sizeof(DE);
	int blocksNeeded = ((bytesNeeded + BLOCK_SIZE -1) / BLOCK_SIZE);
	bytesNeeded = blocksNeeded * BLOCK_SIZE;
	int actualDirEntries = (bytesNeeded / sizeof(DE));
//	printf("byteNeeded: %d, blocksNeeded: %d, actualDirEntries: %d\n", bytesNeeded, blocksNeeded, actualDirEntries);


	EXTENT * tempArray = allocateBlocks(blocksNeeded, blocksNeeded);
	int dirExtentBlock= initExtent(actualDirEntries);
//	printf("In initDir tempArray returned of extent *. length tempArray: %ld, value of tempArray[0] start: %d count: %d\n", 
//	sizeof(tempArray) / sizeof(tempArray[0]), tempArray[0].start, tempArray[0].count);

	if(!tempArray || tempArray==NULL || dirExtentBlock == -1)
		{
		printf("No space for directory.\n");
		p = NULL;
		return -1;
		}

	int dirStart= tempArray[0].start;

        // dir can be used as an array since it is pointer
        struct DE *dir = malloc(bytesNeeded);

	//initiate empty entries 1 to n 
        for (int i = 2; i < actualDirEntries; i++)
                {
                // Initialize each entry to unused
                dir[i].fileName[i]= '\0'; // Null-terminated name
		dir[i].fileSize = 0;
    		dir[i].extentBlockStart= dirExtentBlock;
        	dir[i].extentIndex=i;
                time_t t = time(NULL);
                dir[i].createdTime = t;
                dir[i].modifiedTime = t;
                dir[i].lastAccessedTime = t;
                dir[i].isDirectory = 0;

//                printf("filename: %c\n", dir[i].fileName[i]);
                }

	//initiate entyr 0 and 1
        if (p == NULL)
        {
                // Initialize  . and .. manually

        	// If there's no parent (root directory), set p to the root directory entry
        	p = &dir[0];  // Removed the struct DE * declaration
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

//	        printf("dir[0].extentStart: %d\n", dir[0].extentBlockStart);
//		printf("dir[0].fileName: %s\n", dir[0].fileName);

        	// Initialize ".." entry for root
        	strcpy(dir[1].fileName, "..");
        	dir[1].fileSize = p->fileSize;
       		dir[1].extentBlockStart= dirExtentBlock;
                dir[1].extentIndex=1;
		dir[1].createdTime = p->createdTime;
        	dir[1].modifiedTime = p->modifiedTime;
        	dir[1].lastAccessedTime = p->lastAccessedTime;
        	dir[1].isDirectory = p->isDirectory;

//                printf("dir[1].extentStart: %d\n", dir[1].extentBlockStart);
//		printf("dir[1].fileName: %s\n", dir[1].fileName);

		rootGlobal = dirStart;
		cwdGlobal= rootGlobal;
   		}
    	else
	    	{
//		printf("entered else loop\n");

                // Initialize "." entry
//  		printf("locaiton rootGlobal: block: %d\n", rootGlobal);
                struct DE * rootDirCp = malloc(BLOCK_SIZE);
                int returnCheck= LBAread(rootDirCp, 1, rootGlobal);
//                printf("else root copy returnCheck: %d, rootDir[0].fileName: %s\n", returnCheck, rootDirCp[0].fileName);
                struct DE * r = &rootDirCp[0];

                strcpy(dir[0].fileName, ".");
		dir[0].fileSize= (r->fileSize);
   		dir[0].extentBlockStart= r-> extentBlockStart;
                dir[0].extentIndex=0;
		dir[0].createdTime = r->createdTime;
                dir[0].modifiedTime = r->modifiedTime;
                dir[0].lastAccessedTime = r->lastAccessedTime;
                dir[0].isDirectory = r->isDirectory;
		free(rootDirCp);
		r = NULL;
//              printf("dir[0].extentStart: %d\n", dir[0].extentBlockStart);


//              printf("check subdir[0].fileName: %s\n", dir[0].fileName);
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
//                printf("dir[1].extentStart: %d\n", dir[1].extentBlockStart);
		}


        // Now write it to disk
  	if(LBAwrite(dir, blocksNeeded, dirStart) != blocksNeeded)
		{
            	printf("LBA Write error!\n");
		exit(1);
		}

	if(p) p = NULL;
	free(dir);
	dir = NULL;
	free(tempArray);
	//return start of directory location
	return dirStart;
}
