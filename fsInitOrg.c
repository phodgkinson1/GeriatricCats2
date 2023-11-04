/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Paige Hodgkinson, Jeawan Jang, Carlos Campos Lozano,  Randale Reyes
* Student IDs: 922282852, 923070860, 920768261, 921008696
* GitHub Name: phodgkinson1, jeawanjang, ccamposlozano, RandaleReyes
* Group Name: Geriatric Cats
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"
#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "stdbool.h"

#define BLOCK_SIZE 512
#define SIGNATURE time(NULL)
#define TOTAL_NUM_BLOCKS 19531 // 10,000,000 / 512 = 19531.2
#define DEFAULT_ENTRIES 50

int rootGlobal;

int bytesInBitmap;
unsigned char * bitmapGlobal;

// struct VCB
typedef struct VCB
{
    long signature;         // Signature determines if formatted
    int totalNumBlocks;           //total blocks in entire volume including VCB
    int sizeOfBlock;        // bytes per block (512)
    int startFreeSpaceManagement;
    int startDirectory;
} VCB;


// struct extent
// Includes
// initFreeSpace() loadFreeSpace() allocateBlocks() releaseBlocks()
typedef struct extent
{
    int start;
    int count;
} extent, *pextent;



// struct directory
typedef struct DE
{
    char fileName[32];
    int offset; 		//= i in array for both directory entries array and extents array
    int fileSize;
    time_t createdTime;
    time_t modifiedTime;
    time_t lastAccessedTime;
    char isDirectory;       //0 for file, 1 for directory
} DE;



// Initialize Volume Control Block
void initVCB(struct VCB *vcb)
{
    vcb->signature = SIGNATURE;
    vcb->totalNumBlocks = TOTAL_NUM_BLOCKS;
    vcb->sizeOfBlock = BLOCK_SIZE;
    LBAwrite(vcb, 1, 0);
    // See Below initFileSystem() to initialize startFreeSpaceManagement && startDirectory
}


// InitFreeSpace is called when you initialize the volume
// returns the block number where the freespace map starts
int initFreeSpace (int blockCount, int bytesPerBlock)
{
    // Calculate the size of the bitmap in bytes
    int bitmapSize = blockCount / 8;

    if (bitmapSize % BLOCK_SIZE > 0)
    {
	bitmapSize = BLOCK_SIZE * (bitmapSize / BLOCK_SIZE + 1);
    }

    bytesInBitmap = bitmapSize;
    int freeSpaceBlockCounts = bitmapSize / BLOCK_SIZE;

    // Just for variable check
    printf("bitmapSize: %d\n", bitmapSize); // 2560
    printf("blockCount : %d\n", blockCount); // 19531
    printf("bytesPerBlock: %d\n", bytesPerBlock); // 512
    printf("FreeSpace block counts: %d\n", freeSpaceBlockCounts); // 5

    // Allocate a memory of bitmap based on bitmapSize
    unsigned char * bitmap = malloc(bitmapSize);
    bitmapGlobal = bitmap;

    //initialize every bit on the bitmap to 0
    memset(bitmap, 0, bitmapSize);

    //marked the first 6 bits to used, block 0 for VCB + blocks 1-5 for the bitmap
    for (int i = 0; i < freeSpaceBlockCounts + 1; i++)
    {
	int byteIndex = i / 8; //Find the index of the byte (not user it should be *)								
	int bitIndex =  i % 8; //Find index of bit inside the byte
	uint64_t mask= 1; //mask declared explictly he said to do
	bitmap[byteIndex] |= (mask << bitIndex); //set the bit to one
    }

/*
	printf("bitmap printed, bits order low to high");
	//printbitmap before writing
	for (int i=0; i<bitmapSize; i++)
		{
		printf("[");
		for(int j=0; j <8 ; j++)
			{
			printf("%d", !!((bitmap[i] >> j) & 0x01));
			}
		printf("]");
		}
	printf("|end bitmap \n");
*/
	//write to the disk
    LBAwrite(bitmap, freeSpaceBlockCounts, 1);

    bitmap = NULL;
    //return the block where the free space starts
    return 1;
}


// If the volume is already initialized you need to call loadFreeSpace so the systems
// has the freespace system ready to use
int loadFreeSpace (int blockCount, int bytesPerBlock)
{
    // This function is called when the volume is already initialized !!
    // Calculate size of bitmap in bytes again (same concept just as initFreeSpace)
    // Again if default blockCount is 19531, divided by 8 is 2441
    // 2441 % 5 > 0, so round up, --> 4 + 1 = 5 
    int bitmapSize = blockCount / 8;

    if (bitmapSize % BLOCK_SIZE > 0)
    {
        bitmapSize = BLOCK_SIZE * (bitmapSize / BLOCK_SIZE + 1); // --> 5
    }
    bytesInBitmap = bitmapSize;

    // Allocate memory for the bitmap
    unsigned char * bitmap = malloc(bitmapSize);

    // Based on the bitmapSize, find the freeSpaceBlockCounts like previous step
    int freeSpaceBlockCounts = bitmapSize / BLOCK_SIZE;

    // Just for variable check
    printf("bitmapSize: %d\n", bitmapSize); // 2560
    printf("blockCount : %d\n", blockCount); // 19531
    printf("BLOCK_SIZE= bytesPerBlock: %d\n", bytesPerBlock); // 512
    printf("FreeSpace block counts: %d\n", freeSpaceBlockCounts); // 5

    // Read the free space map from the disk
    // read from block 1 until 5 blocks
    LBAread(bitmap, freeSpaceBlockCounts, 1);

    printf("bitmap printed, bits order low to high: ");

    //printbitmap after reading into buffer
/*      for (int i=0; i<bitmapSize; i++)
                {
                printf("[");
                for(int j=0; j <8 ; j++)
                        {
                        printf("%d", !!((bitmap[i] >> j) & 0x01));
                        }
                printf("]");
                }
        printf("|end bitmap \n");
*/
    bitmap = NULL;
    // ready to use free space system
    return 1;
}


// These are the key functions that callers need to use to allocate disk blocks and free them
// allocateBlocks is how you obtain disk blocks. The first parameter is the number of blocks
// the caller requires. The second parameter is the minimum number of blocks in any one extnet.
// If the two parameters are the same value, then the resulting allocation will be contiguous
// in one extent. The return value is an array of extents the last one has the start and count = 0
// which indicates the end of the array of extents.
// The caller is reponsible for calling free on the return value;
struct extent * allocateBlocks (int required, int minPerExtent)
{
    printf("allocateBlocks() called\n");
    int numExtents= required / minPerExtent;
    // Allocate memory for the array extents
    struct extent * tempArray= malloc(numExtents*sizeof(struct extent *));

    int total = required;
    int start = -1;
    int count = 0;
    int value;
    int tempArrayIndex = 0;

    printf("required: %d blocks, minperextent: %d blocks\n", required, minPerExtent);

    for (int i=0; i<bytesInBitmap; i++)
    {
        for(int j=0; j < 8; j++)
        {
            value= !!((bitmapGlobal[i] >> j) & 0x01);
            if (value==0)
	    {
	        if(start == -1)
		{
		    start= i*8 + j;		//current bit number
		    count = 1;		//count is 1 block
		}
		else				//start at a number
		{
		    count++;
		}
		//one full extent

	        if(minPerExtent == count)
                {
		    struct extent e;
		    e.start = start;
		    e.count = count;
                    tempArray[tempArrayIndex] = e;
                    printf("extent made ->start: %d, ->count: %d\n", tempArray[tempArrayIndex].start, tempArray[tempArrayIndex].count);
		    tempArrayIndex++;
		    total -= count;
                    if(total==0) break;  // replace with goto
                }
	    }
	    //bit is 1
	    else
	    {
		start= -1;
		count = 0;
	    }
	}  //inner bit loop

 	int block;
        int tempByte;
        int tempBitIndex;
        int exitByte;
        int exitBit;
        uint64_t mask= 1;
 
	if(total==0) 
        {
            //mark fsm
	    //use byte and bit value as inner loop starts, and use the count as loop exit condition
	    for (int i = 0; i< tempArrayIndex; i++)
	    {
		block = tempArray[i].start;
		tempByte= block / 8;
		tempBitIndex =  block % 8;
		exitByte= (block+ tempArray[i].count) / 8;
		exitBit= (block+ tempArray[i].count) % 8;

		printf("block: %d, tempByte: %d, tempBitIndex: %d, exitByte: %d, exitBit: %d\n", 
		block, tempByte, tempBitIndex, exitByte, exitBit);

                //loop to check first byte

		if(tempByte < exitByte)
		{
		    for(int i= tempBitIndex; i < 8; i++)
		    {
		        bitmapGlobal[tempByte] |= (mask << i); //set the bits to 1
		    }
		}
		//tempByte== exitbyte
		else
		{
		    for(int i= tempBitIndex; i <= exitBit ; i++)
                    {
                        bitmapGlobal[tempByte] |= (mask << i); //set the bits to 1
                    }
		}
                //loop for center bytes
		if(tempByte < exitByte+ 1)
		{
		    for(int j = tempByte +1; j < exitByte; j++)
		    {
			for(int k =0; k < 8; k++)
			{
			     bitmapGlobal[j] |= (mask << k); //set the bits to 1
			}
		    }
		}
                //loop to check last byte block
    		if(tempByte < exitByte)
                {
                    for(int i= 0; i < exitBit; i++)
                    {
                        bitmapGlobal[exitByte] |= (mask << i); //set the bits to 1
                    }
                }
				}
                // end tempArray loop
/*
			printf("bitmap printed, bits order low to high: ");
        		//printbitmap allocateblock marks FSM
 		      	for (int i = 0; i <= exitByte; i++)
                		{
                		printf("[");
                		for(int j=0; j < 8 ; j++)
                        		{
                        		printf("%d", !!((bitmapGlobal[i] >> j) & 0x01));
                        		}
                		printf("]");
                		}
        		printf("|end bitmap \n");
*/
		break;
	}
    }//outer byte loop
    if(total > 0)
    {
        printf("no space for %d blocks\n", required);
	free(tempArray);
	tempArray=NULL;
    }
    return tempArray;
}


// This function returns blocks to the freespace system. If the caller wants to free all
// the blocks in a series of extents, they should loop through each extent calling releaseBlocks
// for each extent
void releaseBlocks (int start, int count)
{


}


// Initialize Directory
// Does not work unless extent *allocateBlock() function is implemented and extent table works
// Also de * --> DE array in pointer type *****
int initDir(int defaultEntries, struct DE * parent)
{
    struct DE *p = parent;  // Declare p here to make it accessible in the if-else blocks
    printf("p filename: %s\n", p->fileName);

    // printf("size Directory entry: %ld\n", sizeof(struct DE));
    // fill parent given (".") with location returned from allocateBlocks- should be block 7
    printf("\nINITDIR CALLED\n");

    int bytesNeeded = defaultEntries * sizeof(DE);
    int blocksNeeded = ((bytesNeeded + BLOCK_SIZE -1) / BLOCK_SIZE);
    bytesNeeded = blocksNeeded * BLOCK_SIZE;
    int actualDirEntries = (bytesNeeded / sizeof(DE));

    printf("byteNeeded: %d, blocksNeeded: %d, actualDirEntries: %d\n", bytesNeeded, blocksNeeded, actualDirEntries);

    struct extent * tempArray = allocateBlocks(blocksNeeded, blocksNeeded);
    // printf("In initDir tempArray returned of extent *. length tempArray: %ld, value of tempArray[0] start: %d count: %d\n", 
    // sizeof(tempArray) / sizeof(tempArray[0]), tempArray[0].start, tempArray[0].count);

    if(!tempArray || tempArray==NULL)
    {
	printf("No space for directory.\n");
	p = NULL;
	return -1;
    }

    int dirStart= tempArray[0].start;

    // dir can be used as an array since it is pointer
    struct DE *dir = malloc(bytesNeeded);

    // added a check for tempArray before this section
    for (int i = 2; i < actualDirEntries; i++)
    {
        // Initialize each entry to unused
        dir[i].fileName[i]= '\0'; // Null-terminated name
	dir[i].fileSize = bytesNeeded;
        dir[i].offset= i;
        time_t t = time(NULL);
        dir[i].createdTime = t;
        dir[i].modifiedTime = t;
        dir[i].lastAccessedTime = t;
        dir[i].isDirectory = 0;

    // printf("filename: %c\n", dir[i].fileName[i]);
    }

    if (p == NULL)
    {
        // Initialize  . and .. manually

        // If there's no parent (root directory), set p to the root directory entry
        p = &dir[0];  // Removed the struct DE * declaration
        // Initialize "." entry
        strcpy(p->fileName, ".");
        p->fileSize = 0;
        p->offset = 0;
        time_t t = time(NULL);
        p->createdTime = t;
        p->modifiedTime = t;
        p->lastAccessedTime = t;
        p->isDirectory = 1;

	printf("dir[0].fileName: %s\n", dir[0].fileName);

        // Initialize ".." entry for root
        strcpy(dir[1].fileName, "..");
        dir[1].fileSize = p->fileSize;
        dir[1].offset= 1;
       	dir[1].createdTime = p->createdTime;
        dir[1].modifiedTime = p->modifiedTime;
        dir[1].lastAccessedTime = p->lastAccessedTime;
        dir[1].isDirectory = p->isDirectory;

	printf("dir[1].fileName: %s\n", dir[1].fileName);

	rootGlobal = dirStart;
    }
    else
    {
        // printf("entered else loop\n");

        // Initialize "." entry
        printf("locaiton rootGlobal: block: %d\n", rootGlobal);

        struct DE * rootDirCp = malloc(BLOCK_SIZE);
        int returnCheck= LBAread(rootDirCp, 1, rootGlobal);

        printf("else root copy returnCheck: %d, rootDir[0].fileName: %s\n", returnCheck, rootDirCp[0].fileName);

        struct DE * r = &rootDirCp[0];

        strcpy(dir[0].fileName, ".");
        dir[0].fileSize= (r->fileSize);
        // dir[0]->extBlock= r->extBlock;
	dir[0].offset=0;
	dir[0].createdTime = r->createdTime;
        dir[0].modifiedTime = r->modifiedTime;
        dir[0].lastAccessedTime = r->lastAccessedTime;
        dir[0].isDirectory = r->isDirectory;
	free(rootDirCp);
	r = NULL;

        printf("check subdir[0].fileName: %s\n", dir[0].fileName);

        // Initialize ".." entry for root AS PARENT PASSED
        strcpy(dir[1].fileName, "..");
        dir[1].fileSize = bytesNeeded;
        // dir[1].extBlock= ______; new ext block returned- initExtentBlock called in this else condition
        dir[1].offset= 1;
        time_t t = time(NULL);
        dir[1].createdTime = t;
        dir[1].modifiedTime = t;
        dir[1].lastAccessedTime = t;
        dir[1].isDirectory = 1;
        printf("dir[1].fileName: %s at block %d subdirectory \n", dir[1].fileName, dirStart);
    }


    // Now write it to disk

    int returnCheck= LBAwrite(dir, blocksNeeded, dirStart);
    //printf("value returnCheck after write: %d\n", returnCheck );

    if(p) p = NULL;
    free(dir);
    dir = NULL;
    free(tempArray);

    //return start of directory location
    return dirStart;
}


// Fundamental function that initialize the File System
// including initVCB(), initFreeSpace(), and initDir()
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
{
    printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    //initialize a buffer to check for volume control block
    struct VCB *vcb = malloc(BLOCK_SIZE * sizeof(char));
    struct DE * de = malloc(BLOCK_SIZE);

    // LBAread vcb from [0] for 1 block, VCB starts from 0 and is one block
    LBAread(vcb, 1, 0);

    // Check whether the signature is matched, if true volume is already initialized
    if (vcb->signature == SIGNATURE)
    {
	printf("signature: %ld\n", vcb->signature);
	printf("SIGNATURE: %d\n", SIGNATURE);
	// Update already done
	printf("\n\nVolume is already initialized\n");

	// call/return check loadFreeSpace to copy the existing freespace
        int loadFS = loadFreeSpace(numberOfBlocks, blockSize);
        printf("start FSM: %d\n", loadFS);
    }
    //initialize volume
    else
    {
        printf("signature: %ld\n", vcb->signature);
        printf("SIGNATURE: %d\n", SIGNATURE);
	// Update needed
	printf("\n\n Entered condition to initialize volume \n");

        //  *****  Initialize File System *****
	// (1) Initialize the values in the VCB (volume control block)
	initVCB(vcb);

	printf("***** After Initializing VCB ***** \n");
        printf("signature: %ld\n", vcb->signature);
	printf("total num blocks: %d\n", vcb->totalNumBlocks);
	printf("block size: %d\n", vcb->sizeOfBlock);

	free(vcb);

	// (2) Initialize free space management
	int startFreeSpaceManagement = initFreeSpace(numberOfBlocks, blockSize);
	printf("start FSM: %d\n",startFreeSpaceManagement);

	// (3) Initialize root directory
        int startDirectory = initDir(DEFAULT_ENTRIES, NULL);
        printf("startDirectory: %d\n", startDirectory);

	int returnCheck= LBAread(de, 1, 6);
	printf("returnCheck: %d, de[0].fileName: %s\n", returnCheck, de[0].fileName);

	int subDirReturn = initDir(DEFAULT_ENTRIES, *&de);
	printf("subDirReturn: %d\n", subDirReturn);

	free(de);
    }

    return 0;
}


void exitFileSystem ()
{
    free(bitmapGlobal);
    printf ("System exiting\n");
}
