#ifndef VCB_H
#define VCB_H

#include <sys/types.h>


#include "fsLow.h"
#include "mfs.h"
#include "stdio.h"
#include "time.h"
#include "stdlib.h"
#include "stdbool.h"

#define BLOCK_SIZE 512
#define TOTAL_NUM_BLOCKS 19531 // 10,000,000 / 512 = 19531.2
#define SIGNATURE time(NULL)


// struct VCB
typedef struct VCB {
    long signature;         // Signature determines if formatted
    int totalNumBlocks;     // total blocks in entire volume including VCB
    int sizeOfBlock;        // bytes per block (512)
    int startFreeSpaceManagement;
//    int startDirectory;
} VCB;

void initVCB(VCB *vcb);

#endif // VCB_H
