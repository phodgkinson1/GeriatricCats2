#include "freespace.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
//	printf("bitmapSize: %d\n", bitmapSize); // 2560
//      printf("blockCount : %d\n", blockCount); // 19531
//	printf("bytesPerBlock: %d\n", bytesPerBlock); // 512
//	printf("FreeSpace block counts: %d\n", freeSpaceBlockCounts); // 5

	// Allocate a memory of bitmap based on bitmapSize
	unsigned char * bitmap = malloc(bitmapSize);
	bitmapGlobal = bitmap;

	//initialize every bit on the bitmap to 0
	memset(bitmap, 0, bitmapSize);

	//marked the first 6 bits to used, block 0 for VCB + blocks 1-5 for the bitmap
	for (int i = 0; i < freeSpaceBlockCounts + 1; i++)
		{
		int byteIndex = i / 8; //Find the index of the byte
		int bitIndex =  i % 8; //Find index of bit inside the byte
		uint64_t mask= 1;
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
/*
  	if( LBAwrite(bitmap, freeSpaceBlockCounts, 1) !=  freeSpaceBlockCounts)
                {
                printf("LBAwrite error!\n");
                exit(1);
				}
  */              

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
//        printf("bitmapSize: %d\n", bitmapSize); // 2560
//        printf("blockCount : %d\n", blockCount); // 19531
//        printf("BLOCK_SIZE= bytesPerBlock: %d\n", bytesPerBlock); // 512
//        printf("FreeSpace block counts: %d\n", freeSpaceBlockCounts); // 5


	// Read the free space map from the disk
	// read from block 1 until 5 blocks
	if(LBAread(bitmap, freeSpaceBlockCounts, 1) != 1)
                {
                printf("LBAread() error!\n");
                exit(1);
                }
/*
 	 printf("bitmap printed, bits order low to high: ");
        //printbitmap after reading into buffer
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
	bitmap = NULL;
	// ready to use free space system
	return 1;
}


//init and extent block for every directory
int initExtent(int entries)
	{
	printf("initExtent() called with %d entries!\n", entries);
//	printf("sizeof extTable %ld\n", sizeof(extTable));
	int bytesNeeded= entries * sizeof(EXTTABLE);
//      printf("byteNeeded: %d\n", bytesNeeded);
	int blocksNeeded = ((bytesNeeded + BLOCK_SIZE -1) / BLOCK_SIZE);
        bytesNeeded = blocksNeeded * BLOCK_SIZE;
//      printf(" after rounded blocks byteNeeded: %d, blocksNeeded: %d\n", bytesNeeded, blocksNeeded);

        EXTENT * tempArray = allocateBlocks(blocksNeeded, blocksNeeded);
	int extentTableStart= tempArray[0].start;
	free(tempArray);
	printf("extTableStart: %d\n", extentTableStart);

	if(extentTableStart == -1)
		{
		printf("No room for extent table\n");
		return -1;
		}

	EXTTABLE * extentBlock = malloc(bytesNeeded);

/*
	//extent table fill/print check routine
	for (int i = 0; i < entries; i++)
		{
		printf("table %d [\n", i);
		for (int j=0; j < 5; j++)
			{
			extentBlock[i].tableArray[j].start = j;
                        printf("entry %d start: %d\n", j, extentBlock[i].tableArray[j].start);
			}
		printf("]\n");
		}
*/
	if(LBAwrite(extentBlock, blocksNeeded, extentTableStart) != blocksNeeded)
		{
		printf("LBAwrite() error!\n");
		exit(1);
		}

	free(extentBlock);
	return extentTableStart;
	}


// These are the key functions that callers need to use to allocate disk blocks and free them
// allocateBlocks is how you obtain disk blocks. The first parameter is the number of blocks
// the caller requires. The second parameter is the minimum number of blocks in any one extnet.
// If the two parameters are the same value, then the resulting allocation will be contiguous
// in one extent. The return value is an array of extents the last one has the start and count = 0
// which indicates the end of the array of extents.
// The caller is reponsible for calling free on the return value;
// ****** TODO NOT SURE HERE && DID NOT CHECK .... ******

EXTENT * allocateBlocks (int required, int minPerExtent)
	{
//	printf("allocateBlocks() called\n");
	int numExtents= required / minPerExtent;
        // Allocate memory for the array extents
	EXTENT * tempArray= malloc(numExtents*sizeof(struct extent *));

	int total = required;
	int start = -1;
	int count = 0;
	int value;
	int tempArrayIndex = 0;

//	printf("required: %d blocks, minperextent: %d blocks\n", required, minPerExtent);

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
					EXTENT e;
					e.start = start;
					e.count = count;
                                        tempArray[tempArrayIndex] = e;
//                                        printf("extent made ->start: %d, ->count: %d\n", tempArray[tempArrayIndex].start, tempArray[tempArrayIndex].count);
					tempArrayIndex++;
					total -= count;
					if(total==0) break;											// replace with goto
                                       	}
				}
			//bit is 1
			else
				{
					start= -1;
					count = 0;
				}
		}//inner bit loop


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

//				printf("block: %d, tempByte: %d, tempBitIndex: %d, exitByte: %d, exitBit: %d\n", 
//				block, tempByte, tempBitIndex, exitByte, exitBit);

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
				}// end tempArray loop
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
void releaseBlocks(int start, int count)
{
    printf("Releasing %d blocks starting at block %d\n", count, start);

 			int block;
                        int tempByte;
                        int tempBitIndex;
                        int exitByte;
                        int exitBit;
                        uint64_t mask= 1;

      //use byte and bit value as inner loop starts, and use the count as loop exit condition
                                block = start;
                                tempByte= block / 8;
                                tempBitIndex =  block % 8;
                                exitByte= (block+ count) / 8;
                                exitBit= (block+ count) % 8;
                              printf("block: %d, tempByte: %d, tempBitIndex: %d, exitByte: %d, exitBit: %d\n", 
                              block, tempByte, tempBitIndex, exitByte, exitBit);

	//loop to check first byte

                                if(tempByte < exitByte)
                                        {
                                        for(int i= tempBitIndex; i < 8; i++)
                                                {
                                                bitmapGlobal[tempByte] &= ~(mask << i); //set the bits to 1
                                                }
                                        }
                                //tempByte== exitbyte
                                else
                                        {
                                         for(int i= tempBitIndex; i <= exitBit ; i++)
                                                {
                                                bitmapGlobal[tempByte] &= ~(mask << i); //set the bits to 1
                                                }
                                        }

//loop for center bytes
                                if(tempByte < exitByte+ 1)
                                        {
                                        for(int j = tempByte +1; j < exitByte; j++)
                                                {
                                                for(int k =0; k < 8; k++)
                                                        {
                                                        bitmapGlobal[j] &= ~(mask << k); //set the bits to 1
                                                        }
                                                }
//loop to check last byte block

                                if(tempByte < exitByte)
                                        {
                                        for(int i= 0; i < exitBit; i++)
                                                {
                                                bitmapGlobal[exitByte] &= ~(mask << i); //set the bits to 1
                                                }
                                        }
                                }// end tempArray loop
/*
    // Print the updated bitmap
    printf("Bitmap after releasing blocks: ");
    for (int i = 0; i < bytesInBitmap; i++) 
		{
        	printf("[");
        	for (int j = 0; j < 8; j++) 
			{
            		printf("%d", !!((bitmapGlobal[i] >> j) & 0x01));
        		}
        	printf("]");
    		}
    		printf("| End bitmap\n");
*/
}
