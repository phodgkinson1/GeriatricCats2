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

#include "vcb.h"
#include "freespace.h"
#include "directoryEntry.h"
#include "mfsHelper.h"

#define SIGNATURE time(NULL)

// Fundamental function that initialize the File System
// including initVCB(), initFreeSpace(), and initDir()
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
{
    printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    //initialize a buffer to check for volume control block
    struct VCB *vcb = malloc(BLOCK_SIZE * sizeof(char));
  //  struct DE * de = malloc(BLOCK_SIZE);

    // LBAread vcb from [0] for 1 block, VCB starts from 0 and is one block
 	if( LBAread(vcb, 1, 0) !=  1)
                {
                printf("LBAread vcb error!\n");
                exit(1);
                }


    // Check whether the signature is matched, if true volume is already initialized
    if (vcb->signature == SIGNATURE)
    	{
//	printf("signature: %ld\n", vcb->signature);
//	printf("SIGNATURE: %ld\n", SIGNATURE);
	// Update already done
	printf("\n\nVolume is already initialized\n");

	// call/return check loadFreeSpace to copy the existing freespace
        int loadFS = loadFreeSpace(numberOfBlocks, blockSize);
//        printf("start FSM: %d\n", loadFS);
    	}
    //initialize volume
    else
    	{
//        printf("signature: %ld\n", vcb->signature);
//        printf("SIGNATURE: %ld\n", SIGNATURE);
	// Update needed
//	printf("\n\n Entered condition to initialize volume \n");

        //  *****  Initialize File System *****
	// (1) Initialize the values in the VCB (volume control block)
	initVCB(vcb);

//	printf("***** After Initializing VCB ***** \n");
//        printf("signature: %ld\n", vcb->signature);
//	printf("total num blocks: %d\n", vcb->totalNumBlocks);
//	printf("block size: %d\n", vcb->sizeOfBlock);

	// (2) Initialize free space management
	int fsmReturn = initFreeSpace(numberOfBlocks, blockSize);
//	printf("start FSM: %d\n",startFreeSpaceManagement);
	vcb->startFreeSpaceManagement= fsmReturn;

    	//write vcb
         if( LBAwrite(vcb, 1, 0) != 1)
                {
                printf("LBAwrite error!\n");
                exit(1);
                }

        if(vcb) free(vcb);
	vcb=NULL;

	// (3) Initialize root directory
      	int startDirectory = initDir(DEFAULT_ENTRIES, NULL, 0);
//        printf("startDirectory: %d\n", startDirectory);

//	int returnCheck= LBAread(de, 1, 6);
//	printf("returnCheck: %d, de[0].fileName: %s\n", returnCheck, de[0].fileName);

//	int subDirReturn = initDir(DEFAULT_ENTRIES, *&de);
//	printf("subDirReturn: %d\n", subDirReturn);

//test only	releaseBlocks(0, 2);
//	free(de);
    	}

    	return 0;
}


void exitFileSystem ()
{
	//write freespace
        int freeSpaceBlockCounts = bytesInBitmap / BLOCK_SIZE;

	//NOTE! if hardcoding position at block 1 is improper, we can keep vcb alive 
	//In a global variable to reference vcb->startFreeSpaceManagement and then 
	//write/free vcb here.
	 if( LBAwrite(bitmapGlobal, freeSpaceBlockCounts, 1) !=  freeSpaceBlockCounts)
                {
                printf("LBAwrite error!\n");
                exit(1);
                }

        if (bitmapGlobal != NULL) free(bitmapGlobal);//release freespace
	bitmapGlobal=NULL;
	if(cwd != NULL) free(cwd);
	if(rootDir != NULL) free(rootDir);
    	printf ("System exiting\n");
}
