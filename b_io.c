/**************************************************************
 * Class:  CSC-415-0# Fall 2021
 * Names:
 * Student IDs:
 * GitHub Name:
 * Group Name:
 * Project: Basic File System
 *
 * File: b_io.c
 *
 * Description: Basic File System - Key File I/O Operations
 *
 **************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define START_ALLOCATION 2

typedef struct b_fcb
{
	// int allocation;
	int permissions;
	DE *parent;
	EXTTABLE *parentExtent;
	int allocation;
	int dirIndex;
	struct fs_diriteminfo *fi; // holds the low level systems file info in assign 5- do we need this though? we could also just keep dir Entry pointer here.
							   // figure out what we use from orig assign 5 low level ptr info. then replace with info through ptrs.
	b_io_fd fd;
	char *fileBuffer;
	char *rdBuffer;
	int rdBufferIndex;
	int fileBlock;
	int fp;
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; // Indicates that this has not been initialized

// Method to initialize our file system
void b_init()
{
	// init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].fd = -1; // indicates a free fcb
	}

	startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].fd == -1)
		{
			return i; // Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); // all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
/*
linux below
#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100
#define O_TRUNC		00001000
#define O_APPEND	00002000

*/
b_io_fd b_open(char *filename, int flags)
{
	b_io_fd returnFd;
	DE *currentFile;

	if (startup == 0)
		b_init(); // Initialize our system

	if ((flags | O_RDONLY) == O_RDONLY)
	{
		printf("passed O_RDONLY!\n");
	}
	if ((flags & O_WRONLY) == O_WRONLY)
	{
		printf("passed O_WRONLY!\n");
	}

	if ((flags & O_RDWR) == O_RDWR)
	{
		printf("passed O_WRONLY!\n");
	}

	if ((flags & O_CREAT) == O_CREAT)
	{
		printf("passed O_CREAT!\n");
	}
	if ((flags & O_TRUNC) == O_TRUNC)
	{
		printf("passed O_TRUNC!\n");
	}

	printf("b_open received filename: %s\n", filename);

	char *newPath = pathUpdate(filename);
	printf("newPath: %s\n", newPath);

	if (filename == NULL)
		return -1;

	parsePathInfo *ppi = malloc(sizeof(parsePathInfo));
	int pathValidity = parsePath(newPath, ppi);

	printf("pathValidity: %d, ppi->indexOfLastElement: %d\n",
		   pathValidity, ppi->indexOfLastElement);
	// ppiTest->lastElement[0]= 'E';test for structure passing
	// parsePath returns 0 if valid path for a directory, -2 if invalid

	// bad path
	if (pathValidity != 0)
	{
		printf("invalid pathname\n");
		return -1;
	}
	// if found, cannot be directory
	if (ppi->indexOfLastElement != -1 && isDirectory(&ppi->parent[ppi->indexOfLastElement]))
	{
		printf("pathname is a directory.\n");
		return -1;
	}
	// if valid path, and not found, AND CREAT not specified, return -1
	if (ppi->indexOfLastElement == -1 && (flags & O_CREAT) != O_CREAT)
	{
		printf("file not found, no create permissions\n");
		return -1;
	}

	// at this point, only options left are create or load
	// get the file block to update it
	returnFd = b_getFCB(); // get our own file descriptor
	if (returnFd == -1)
	{
		printf("All file descriptors in use!\n");
		return -1;
	}

	fcbArray[returnFd].fd = returnFd;
	//	printf("file handle: %d\n", returnFd);

	// create a file
	if (ppi->indexOfLastElement == -1 && (flags & O_CREAT) == O_CREAT)
	{
		printf("condition to create a file!\n");

		char *empty = malloc(1);
		empty[0] = '\0';
		int nextAvailable = FindEntryInDir(ppi->parent, empty);
		if (nextAvailable == -1)
		{
			printf("Directory at capacity.\n");
			fcbArray[returnFd].fd = -1;
			return -1;
		}
		free(empty);
		printf("nextAvailable in directory: %d\n", nextAvailable);

		currentFile = &(ppi->parent[nextAvailable]);

		// update directory entry name for new file
		int i = 0;
		while (ppi->lastElement[i])
		{
			currentFile->fileName[i] = ppi->lastElement[i];
			i++;
		}
		printf("filename: |%s| added for dir[%d]\n", currentFile->fileName, nextAvailable);

		// update fcb
		fcbArray[returnFd].dirIndex = nextAvailable;
	} // end create section

	// load a file
	if (ppi->indexOfLastElement != -1)
	{
		printf("condition to load a file!\n");
		currentFile = &(ppi->parent[ppi->indexOfLastElement]);

		// update fcb
		fcbArray[returnFd].dirIndex = ppi->indexOfLastElement;

		// if truncate, release all blocks inside extent tables and set filesize to 0
		if ((flags & O_TRUNC) == O_TRUNC)
		{
			printf("truncating file\n");
			EXTTABLE *ext = loadExtent(currentFile);
			// load extent new file
			if (ext == NULL)
			{
				currentFile = NULL;
				printf("loadExtent() error\n");
				fcbArray[returnFd].fd = -1;
				return -1;
			}
			fcbArray[returnFd].parentExtent = ext;
			printf("extent for file says start is: %d\n", ext[ppi->indexOfLastElement].tableArray[0].start);
			/* itr array of extents- max in system is 5 hardcoded,
			but could do helper function that checks last extent for positive start and count of 0,
	and continues itr in next extent table stored at start.
	*/
			for (int i = 0; i < 5; i++)
			{
				ext[ppi->indexOfLastElement].tableArray[i].start = 0;
				ext[ppi->indexOfLastElement].tableArray[i].count = 0;
				printf("ext array[%d] start: %d count: %d\n", i, ext[ppi->indexOfLastElement].tableArray[i].start,
					   ext[ppi->indexOfLastElement].tableArray[i].count);
			}

		} // end truncate
	}	  // end load file section

	// update fcb
	fcbArray[returnFd].permissions = flags;
	fcbArray[returnFd].parent = ppi->parent;
	fcbArray[returnFd].fileBuffer = malloc(B_CHUNK_SIZE * sizeof(char));
	//              printf("fcb[fd].parent[nextavail].fileName: |%s|\n", fcbArray[returnFd].parent[nextAvailable].fileName);
	//              printf("access test fcbArray[fd].parentExt[nextAvailable].tableArray[0].start : %d\n",
	//              fcbArray[returnFd].parentExt[nextAvailable].tableArray[0].start);

	return (returnFd); // all set
}

// Interface to seek function
int b_seek(b_io_fd fd, off_t offset, int whence)
{
	if (startup == 0)
		b_init(); // Initialize our system

	printf("not written, does nothing yet.\n");
	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		b_close(fd);
		return (-1); // invalid file descriptor
	}

	int fileSize = fcbArray[fd].parent[fcbArray[fd].dirIndex].fileSize;

	off_t new_fp;

	if (whence == SEEK_SET)
	{
		new_fp = offset;
	}
	else if (whence == SEEK_CUR)
	{
		new_fp = fcbArray[fd].fp + offset;
	}
	else if (whence == SEEK_END)
	{
		new_fp = fileSize + offset;
	}
	else
	{
		return -1;
	}

	fcbArray[fd].fp = new_fp;

	return (0); // Change this
}

// Interface to write function
int b_write(b_io_fd fd, char *buffer, int count)
{
	if (startup == 0)
		b_init(); // Initialize our system
	printf("passed fd= %d, count: %d\n", fd, count);

	if (buffer == NULL)
	{
		printf("buffer passed is invalid\n");
		return (-1);
	}

	b_fcb *fcb = &fcbArray[fd];

	// exit if read only
	if ((fcb->permissions | O_RDONLY) == O_RDONLY)
	{
		printf("No write permissions\n");
		return (-1);
	}

	// if append go to end of file
	if ((fcb->permissions | O_APPEND) == O_APPEND)
	{
		b_seek(fd, fcb->parent[fcb->dirIndex].fileSize, SEEK_SET);
	}

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{

		return (-1); // invalid file descriptor
	}

	// load extent for current file if not loaded in open's create condition
	EXTTABLE *ext;

	if (fcb->parentExtent == NULL)
	{
		ext = loadExtent(fcb->parent);
		if (ext == NULL)
		{
			printf("loadExtent() error\n");
			b_close(fcb->fd);
			return -1;
		}
		fcb->parentExtent = ext;
	}

	printf("extent for file says start is: %d\n", fcb->parentExtent[fcb->dirIndex].tableArray[0].start);

	int userCount = count;
	int userBlocks;
	fcb->allocation = START_ALLOCATION;
	int currentBlock = 0;
	int i = 0;

	while (userBlocks > 0)
	{
		userBlocks = ((userCount + BLOCK_SIZE - 1) / BLOCK_SIZE);
		printf("userCount: %d\n", userBlocks);
		int extStart = fcb->parentExtent[fcb->dirIndex].tableArray[i].start;
		int extCount = fcb->parentExtent[fcb->dirIndex].tableArray[i].count;
		// if must allocate space
		if ((extCount - currentBlock) < userBlocks)
		{
			printf("entered allocate condition\n");
			int blocksNeeded = userBlocks - extCount;
			printf("blocksNeeded: %d\n", blocksNeeded);

			EXTENT *tempArray = allocateBlocks(blocksNeeded, blocksNeeded);
			if (tempArray == NULL)
			{
				if (ext != NULL)
					ext = NULL;
				b_close(fcb->fd);
				printf("allocateBlocks() error\n");
				return -1;
			}
			// copy allocation info to extent for directory new file
			int tempArraySize = sizeof(tempArray) / sizeof(tempArray[0]);
			int itrMax = 5;
			if (tempArraySize < 5)
				itrMax = tempArraySize;
			/* copy to array of extents- max in system is 5 hardcoded,
			but could do a helper function that inits second extent table,
			stores location of new extent in start and count of 0, and continues
			copying to index of new extent table. Then when reading/writing,
			would check each extent array index for start >0 but count ==0 for next
			extent table to load.
	*/
			EXTTABLE *extArray; // for clarity
			for (int j = i; j < (itrMax + i); j++)
			{
				extArray = &fcb->parentExtent[fcb->dirIndex];
				// if extent contiguous with last element, combine.
				// UNTESTED IF STATEMENT ONLY ELSE TESTED!
				if (j > 0 && (extArray->tableArray[j - 1].start + extArray->tableArray[j - 1].count) == tempArray[j].start - 1)
				{
					printf("new allocation is continuous with old extent\n");
					currentBlock = extArray->tableArray[j - 1].count;
					extArray->tableArray[j - 1].count += tempArray[j].count;
				}
				// not continguous with last extent
				else
				{
					extArray->tableArray[j].start = tempArray[j].start;
					extArray->tableArray[j].count = tempArray[j].count;
					currentBlock = 0;
				}

				printf("ext array[%d] start: %d count: %d\n", j, fcb->parentExtent[fcb->dirIndex].tableArray[j].start,
					   fcb->parentExtent[fcb->dirIndex].tableArray[j].count);
			}
			extArray = NULL;

			// for future improvement- if not requesting continuous file allocation
			//  we must loop through tempArray size starting at j before next loop.

			extStart = fcb->parentExtent[fcb->dirIndex].tableArray[i].start;
			extCount = fcb->parentExtent[fcb->dirIndex].tableArray[i].count;

			int toWrite = extCount - currentBlock;
			userBlocks -= toWrite;
			printf("toWrite: %d, userCount: %d userBlocks: %d\n", toWrite, userCount, userBlocks);
			if (LBAwrite(buffer, toWrite, extStart) != toWrite)
			{
				printf("b_write() LBAwrite() error!\n");
				exit(1);
			}

		} // end if

		else
		{
			if (LBAwrite(buffer, userBlocks, extStart) != userBlocks)
			{
				printf("b_write() LBAwrite() error!\n");
				exit(1);
			}
		}
	} // end while
	fcb->parent[fcb->dirIndex].fileSize += count;
	return (0); // Change this
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count)
{
	if (startup == 0)
		b_init(); // Initialize our system

	// Check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return -1; // Invalid file descriptor
	}

	// Check if the specified FCB is actually in use
	if (fcbArray[fd].fd == -1)
	{
		return -1; // File not open for this descriptor
	}

	char *userBuffer = buffer;
	fcbArray[fd].rdBuffer = fcbArray[fd].fileBuffer;
	fcbArray[fd].rdBuffer += fcbArray[fd].rdBufferIndex;
	int bytesRead = 0;
	int toRead;
	int fileSize = fcbArray[fd].parent[fcbArray[fd].dirIndex].fileSize;
	int fileRemainder = fileSize - fcbArray[fd].fp;

	// Do not read past end of file
	if (count <= fileRemainder)
	{
		toRead = count;
	}
	else
	{
		toRead = fileRemainder;
	}

	// For the first read on the file
	if (fcbArray[fd].fileBuffer[0] == '\0')
	{
		fcbArray[fd].rdBufferIndex = 0;
	}

	// Optimization to skip empty read
	if (count == 0)
	{
		return 0;
	}

	// If file buffer not empty (1st CONDITION)
	if (fcbArray[fd].rdBufferIndex != 0)
	{
		int remainder = B_CHUNK_SIZE - fcbArray[fd].rdBufferIndex - 1;
		int amt;

		if (toRead < remainder)
		{
			amt = toRead;
		}
		else
		{
			amt = remainder;
		}

		// Copy from our buffer to user buffer
		memcpy(userBuffer, fcbArray[fd].rdBuffer, amt);

		// Update all trackers
		userBuffer += amt;
		fcbArray[fd].rdBufferIndex += amt;
		bytesRead += amt;
		toRead -= amt;
	}

	// After remainder of the last buffer, see if can pass whole blocks
	if (toRead >= B_CHUNK_SIZE)
	{
		// Evaluate if can LBAread even blocks of B_CHUNK_SIZE straight to userBuffer to avoid copying
		int blocks = toRead / B_CHUNK_SIZE;
		int readOut = blocks * B_CHUNK_SIZE;

		int blockFetch = LBAread(userBuffer, blocks, (fcbArray[fd].parentExtent[fcbArray[fd].dirIndex].tableArray[0].start));

		if (blockFetch != blocks)
		{
			printf("Could not read blocks to userBuffer\n");
		}

		// Update all trackers
		userBuffer += readOut;
		fcbArray[fd].fileBlock += blocks;
		bytesRead += readOut;
		toRead -= readOut;
	}

	// If partial block after passing whole blocks or finishing the whole buffer from 1st condition
	if (toRead > 0)
	{
		// If filled buffer from the previous call, clear
		if (fcbArray[fd].rdBufferIndex == B_CHUNK_SIZE - 1)
		{
			fcbArray[fd].rdBuffer = fcbArray[fd].fileBuffer;
			memset(fcbArray[fd].fileBuffer, '\0', B_CHUNK_SIZE);
			fcbArray[fd].rdBufferIndex = 0;
		}

		// Fetch a new block to the file buffer
		if (LBAread(fcbArray[fd].rdBuffer, 1, (fcbArray[fd].parentExtent[fcbArray[fd].dirIndex].tableArray[0].start)) != 1)
		{
			printf("Could not read block to fileBuffer\n");
			exit(1);
		}

		// Copy from fileBuffer to the user buffer
		memcpy(userBuffer, fcbArray[fd].rdBuffer, toRead);

		// Update all trackers
		fcbArray[fd].fileBlock++;
		userBuffer += toRead;
		fcbArray[fd].rdBufferIndex += toRead;
		bytesRead += toRead;
		toRead -= toRead;
	}

	// Update the file pointer (fp is the file index)
	fcbArray[fd].fp += bytesRead;

	// read routine should read up until end of an extent count, and then index unless at end of extent table (<5 condition)
	// your goal is to create a proper outer loop to iterate through the extent table as reading to end of each extent

	/*
	the following routine is my assignment 5 function for b_read and it works to expectation. nothing needs to be changed except for
	extent variables

int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use
	if (fcbArray[fd].fi == NULL)		//File not open for this descriptor
		{
		return -1;
		}

		char * userBuffer= buffer;
		fcbArray[fd].rdBuffer=fcbArray[fd].fileBuffer;
	fcbArray[fd].rdBuffer+=fcbArray[fd].rdBufferIndex;
		int bytesRead=0;
		int toRead;
		int fileSize= (fcbArray[fd].fi)->fileSize;
		int fileRemainder= fileSize-fcbArray[fd].fp;

	//do not return past end of file vvv
	if (count<=fileRemainder) toRead=count;
	else toRead=fileRemainder;

	//for first read on file
	if(fcbArray[fd].fileBuffer[0]=='\0') fcbArray[fd].rdBufferIndex=0;

	//optimization to skip empty read
	if(count==0) return 0;

	//if filebuffer not empty- 1st CONDITION
		if(fcbArray[fd].rdBufferIndex!=0)
		{
		int remainder= B_CHUNK_SIZE-(fcbArray[fd].rdBufferIndex)-1;
		int amt;
		if(toRead<remainder) amt=toRead;
				else amt=remainder;
		//copy from our buffer to userbuffer
		memcpy(userBuffer, fcbArray[fd].rdBuffer, amt);
				//update all trackers
		userBuffer+= amt;
				fcbArray[fd].rdBufferIndex += amt;
				bytesRead += amt;
				toRead -=amt;
		}
	//after remainder of last buffer, see if can pass whole blocks
	if(toRead>=B_CHUNK_SIZE)
		{
		//eval if can LBAread even blocks of B_CHUNK_SIZE straight to userBuffer to avoid copying.
				int blocks=toRead/B_CHUNK_SIZE;
		int readOut=blocks*B_CHUNK_SIZE;
				int blockFetch= LBAread(userBuffer, blocks, (fcbArray[fd].fi->location)+(fcbArray[fd].fileBlock));
				if(blockFetch != blocks)
					{
						printf("could not read blocks to userBuffer\n");
						}
		//update all trackers
		userBuffer +=readOut;
		fcbArray[fd].fileBlock +=blocks;
				bytesRead += readOut;
				toRead-= readOut;
		}
	//if partial block after passing whole blocks or finishing whole buffer from 1st condition
	if(toRead>0)
		{
		//if filled buffer from previous call, clear
		if(fcbArray[fd].rdBufferIndex==B_CHUNK_SIZE-1)
			{
			fcbArray[fd].rdBuffer=fcbArray[fd].fileBuffer;
						memset(fcbArray[fd].fileBuffer, '\0', sizeof(fcbArray[fd].fileBuffer));
						fcbArray[fd].rdBufferIndex=0;
			}
			//fetch new block to file Buffer
			if(LBAread(fcbArray[fd].rdBuffer, 1, (fcbArray[fd].fi->location)+(fcbArray[fd].fileBlock)) != 1)
							{
							printf("could not read block to fileBuffer\n");
							exit;
						}
			//copy from fileBuffer to user buffer
			memcpy(userBuffer, fcbArray[fd].rdBuffer, toRead);
				//update all trackers
						fcbArray[fd].fileBlock++;
						userBuffer+=toRead;
			fcbArray[fd].rdBufferIndex += toRead;
						bytesRead += toRead;
						toRead -=toRead;
		}
	//fp file pointer is file index
		fcbArray[fd].fp += bytesRead;

	return bytesRead;
	}//end read

	*/

	return bytesRead; // Change this to bytes read
}

// Interface to Close the file
int b_close(b_io_fd fd) {
    // First, check if the fd is valid
    if (fd < 0 || fd >= MAXFCBS || fcbArray[fd].fd == -1) {
        // Invalid file descriptor or already closed
        return -1;
    }

    // If the file was opened with write permissions, update the file system structures
    if ((fcbArray[fd].permissions & O_WRONLY) == O_WRONLY || (fcbArray[fd].permissions & O_RDWR) == O_RDWR) {
/*
	int dir = (sizeof(DE) * DEFAULT_ENTRIES + BLOCK_SIZE -1)/ BLOCK_SIZE;

        if (LBAwrite(fcbArray[fd].parent, dir, fcbArray[fd].parent[fcbArray[fd].dirIndex].extentBlockStart) != dir) {
            printf("Error writing directory information\n");
            return -1;
        }

	int ext = (sizeof(EXTTABLE) * DEFAULT_ENTRIES + BLOCK_SIZE -1) / BLOCK_SIZE;

	// int ext = (sizeof(EXTTABLE) * DEFAULT_ENTRIES + BLOCK_SIZE -1) / BLOCK_SIZE;
        if (LBAwrite(fcbArray[fd].parentExtent, ext, fcbArray[fd].parentExtent[fcbArray[fd].dirIndex].tableArray[0].start) != ext) {
            printf("Error writing extent table information\n");
            return -1;
        }
*/

        // Write directory information back to the file system
        if (LBAwrite(fcbArray[fd].parent, sizeof(DE), fcbArray[fd].parent[fcbArray[fd].dirIndex].extentBlockStart) != 1) {
            // Handle error in writing directory information
            printf("Error writing directory information\n");
            return -1;
        }

        // Write extent table information back to the file system
        if (LBAwrite(fcbArray[fd].parentExtent, sizeof(EXTTABLE), fcbArray[fd].parentExtent[fcbArray[fd].dirIndex].tableArray[0].start) != 1) {
            // Handle error in writing extent table information
            printf("Error writing extent table information\n");
            return -1;
        }
    }

    // Free allocated resources
    if (fcbArray[fd].parent != NULL) {
        free(fcbArray[fd].parent);
        fcbArray[fd].parent = NULL;
    }
    if (fcbArray[fd].parentExtent != NULL) {
        free(fcbArray[fd].parentExtent);
        fcbArray[fd].parentExtent = NULL;
    }
    if (fcbArray[fd].fileBuffer != NULL) {
        free(fcbArray[fd].fileBuffer);
        fcbArray[fd].fileBuffer = NULL;
    }

    // Reset the file descriptor
    fcbArray[fd].fd = -1;

    return 0;
}

