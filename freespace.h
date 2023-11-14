#ifndef FREESPACE_H
#define FREESPACE_H

#include <sys/types.h>


#include "fsLow.h"
#include "time.h"
#include "stdbool.h"

#define BLOCK_SIZE 512
#define TOTAL_NUM_BLOCKS 19531

extern int bytesInBitmap;
extern unsigned char * bitmapGlobal;

// struct extent
typedef struct EXTENT
{
        int start;
        int count;
} EXTENT, *pextent;

typedef struct EXTTABLE
{
        EXTENT tableArray[5];
} EXTTABLE;

// Function declarations
int initFreeSpace(int blockCount, int bytesPerBlock);
int loadFreeSpace(int blockCount, int bytesPerBlock);
int initExtent(int entries, int dirLocation);
EXTENT * allocateBlocks(int required, int minPerExtent);
void releaseBlocks(int start, int count);

#endif // FREESPACE_H
