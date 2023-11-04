#include "vcb.h"
#include "fsLow.h"
#include "mfs.h"

// Include other necessary headers here

void initVCB(VCB *vcb) {
    vcb->signature = SIGNATURE;
    vcb->totalNumBlocks = TOTAL_NUM_BLOCKS;
    vcb->sizeOfBlock = BLOCK_SIZE;
}
