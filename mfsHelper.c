#include <string.h>
#include <stdlib.h>
#include "freespace.h"
#include "directoryEntry.h"
#include "mfsHelper.h"
#include "vcb.h"

// if -1 then directory or file does not exist
//  ex) /home/studnet/Documents/foo
//      foo (last element) --> Relative path --> parent = cwd

// Helper Functions: (1) FindEntryInDir(), (2) isDirectory(),
int parsePath(char *path, parsePathInfo *ppi)
{
	// check passing
	//	printf("parsepath passing check: path: %s\n", path);
	//	printf("Parsepath pass check: access ppi->lastElement[0]: |%c|\n", ppi-> lastElement[0]);
	// load root dir, getting size first
	if (rootDir == NULL)
	{
		rootDir = loadDir(rootDir, rootGlobal);
	}

	if (cwd == NULL)
		cwd = rootDir;

	// First, check the pathname is a valid string
	if (path == NULL)
		return -1;
	// Second, check whether ppi is valid or not
	if (ppi == NULL)
		return -1;

	// Check the pathname[0] for absolute path
	DE *startDir;
	DE *parent;

	if (path[0] == '/')
	{
		startDir = rootDir;
	}
	else
	{
		startDir = cwd;
	}

	parent = startDir;

	DE * temp;
		printf("parent[0]:|%s| filesize: %d _____ parent[1]: |%s| filesize: %d \n", parent[0].fileName,
		parent[0].fileSize, parent[1].fileName, parent[1].fileSize);

	// Initialize pointers of saveptr && token1
	char *saveptr;
	char *token1 = strtok_r(path, "/", &saveptr);

	if (token1 == NULL)
	{
		if (strcmp(path, "/") == 0)
		{	startDir=NULL;
			ppi->indexOfLastElement = -1;
			ppi->lastElement = NULL;
			ppi->parent = NULL;
			return 0;
		}
		return -1;
	}

	while (token1 != NULL)
	{
		char *token2 = strtok_r(NULL, "/", &saveptr);
		// look for directory of token 2 name in current parent and return index of directory entry
		int index = FindEntryInDir(parent, token1); // 1 Helper function
													// if reached last directory in our path before creating a new one
		if (token2 == NULL)							// at end
		{
			
			ppi->indexOfLastElement = index;
			ppi->parent = parent;
			ppi->lastElement = strdup(token1);
			return 0;
		}

		// index ==-1 if dir of current token is not found in current parent (which is our current loaded directory iterating) then -2 invalid path
		if (index == -1)
			return -2;

		// if index != -1 then item already exists in our directory, but !isDirectory means the item is a FILE and not dir, so -2 invalid path
		if (!isDirectory(&parent[index]))
			return -2; // 2 Helper function

		// load memory for new directory using index in current parent
		temp = loadDir(parent, index);

		// release current parent if not equal to the root directory or current working directory
		// note- we are only in root directory or current directory in first while loop or last while loop.
		if (parent != startDir)
		{
			free(parent);
		}
		// set parent to search to our newly loaded directory, and set next directory name to search in parent
		parent = temp;
		token1 = token2;
	}
}

// Check the whether Directory entry named fileName is existed or not
// Return value: integer
int FindEntryInDir(DE *dir, char *fileName)
{
	// total  divided by struct DE size
	int numEntries = dir[0].fileSize / sizeof(DE);
	//    	printf("numEntries: %d\n", numEntries);
	// Iterate through the whole number of entries
	for (int i = 2; i < numEntries; i++) // **** paige- I changed this to int i=2 from 0
	{
		// x is found -> initialize x as 0
		if (strcmp(dir[i].fileName, fileName) == 0)
			return i;
	}
	return -1; // otherwise, nope
}

// Check whether entry is directory or others
int isDirectory(DE *dir)
{
	if (dir == NULL)
	{
		printf("Empty !\n");
		return -1;
	}

	if (dir->isDirectory == 1) return 1;
	else return 0;
}


// load an extent table for a directory to retreive locations of directory entry items
EXTTABLE *loadExtent(DE *dir)
{
	printf("loadExtent called \n");
	int extStart = dir[1].extentBlockStart;
	int numEntries = dir[1].fileSize / sizeof(DE);
	printf("extent for dir starts at block %d\n", extStart);
	int bytesNeeded = numEntries * sizeof(EXTTABLE);
	int blocksNeeded = ((bytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE);
	bytesNeeded = blocksNeeded * BLOCK_SIZE;

	EXTTABLE *extPtr = malloc(bytesNeeded);
	if (LBAread(extPtr, blocksNeeded, extStart) != blocksNeeded)
	{
		printf("ext loading 1st block LBAread() error!\n");
		exit(1);
	}
	return extPtr;
}

int writeExtent(DE * dir, EXTTABLE * ext)
{
	printf("writeExtent called \n");
	printf("inside writeExtent- extent[2].tableArray[0].start: %d\n",
	ext[2].tableArray[0].start);
	int extStart = dir[1].extentBlockStart;
        int numEntries = dir[1].fileSize / sizeof(DE);
        printf("extent for dir starts at block %d\n", extStart);
        int bytesNeeded = numEntries * sizeof(EXTTABLE);
        int blocksNeeded = ((bytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE);
        bytesNeeded = blocksNeeded * BLOCK_SIZE;
	printf("extent Blocks needed to write : %d blocks\n", blocksNeeded);
	if (LBAwrite(ext, blocksNeeded, extStart) != blocksNeeded)
        	{
                printf("writeExtent() LBAwrite() error!\n");
                exit(1);
        	}


	return 1;
}

int writeDir(DE * dir, int location)
{
	printf("write dir called\n");
	int numEntries = dir[1].fileSize / sizeof(DE);
	printf("writeDir with numEntries: %d\n", numEntries);
        int bytesNeeded = numEntries * sizeof(DE);
        int blocksNeeded = ((bytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE);
        printf("dir[index].fileSize: %d | blocksNeeded: %d\n", dir[1].fileSize, blocksNeeded);
	if (LBAwrite(dir, blocksNeeded, location) != blocksNeeded)
        	{
                printf("writeDir LBAwrite() error!\n");
                exit(1);
        	}
	return 1;
}

// Loads a directory from disk into memory
DE *loadDir(DE *dir, int index)
{
	printf("loadDir called, with index: %d\n", index);
	int startBlock;
	DE * newDir;

	// if loading root
	if (dir == NULL)
	{
		startBlock = index;
     		newDir = malloc(BLOCK_SIZE);
        	if (LBAread(newDir, 1, startBlock) != 1)
			{
                	printf("dir loading 1st block LBAread() error!\n");
                	exit(1);
        		}
        	int rootDirSize = newDir[0].fileSize;
        	int sizeInBlocks = ((rootDirSize + BLOCK_SIZE - 1) / BLOCK_SIZE);
        	printf("fileSize: %d | sizeinblocks: %d\n", rootDirSize, sizeInBlocks);
        	if (sizeInBlocks > 1)
        		{
                	free(newDir);
                	newDir = malloc(sizeInBlocks * BLOCK_SIZE);
                	if (LBAread(newDir, sizeInBlocks, startBlock) != sizeInBlocks)
                		{
                        	printf("dir loading root full LBAread() error!\n");
                        	exit(1);
                		}
        		}

	}

	else
	{
		EXTTABLE *ext = loadExtent(dir);
		printf("filename at index %d in parent %s\n", index, dir[index].fileName);
		startBlock = ext[index].tableArray[0].start;
		printf("extent table returned start of loading dir at: %d\n", startBlock);
		printf("ext[2].tableArray[0].start: %d\n", ext[2].tableArray[0].start);
		if(ext != NULL) free(ext);

        	int numEntries = dir[1].fileSize / sizeof(DE);
        	int bytesNeeded = numEntries * sizeof(DE);
        	int blocksNeeded = ((bytesNeeded + BLOCK_SIZE - 1) / BLOCK_SIZE);
        	bytesNeeded = blocksNeeded * BLOCK_SIZE;
                printf("dir[index].fileSize: %d | sizeInBlocks: %d\n", dir[index].fileSize, blocksNeeded);
                newDir = malloc(bytesNeeded);
                if (LBAread(newDir, blocksNeeded, startBlock) != blocksNeeded)
                	{
                        printf("dir loading full subdir LBAread() error!\n"); 
                        exit(1);
                        }
		printf("inside loadDir, extentBlockStart at newDir: %d\n", newDir[1].extentBlockStart);
	}
	if(dir != NULL) free(dir);
	return newDir;
}
