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
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
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
//int allocation;
	int permissions;
	DE * parent;
	EXTTABLE * parentExt;
	int dirIndex;
	struct fs_diriteminfo * fi;	//holds the low level systems file info in assign 5- do we need this though? we could also just keep dir Entry pointer here.
					//figure out what we use from orig assign 5 low level ptr info. then replace with info through ptrs.
	b_io_fd fd;
	char * fileBuffer;
	char * rdBuffer;
	int rdBufferIndex;
	int fileBlock;
	int fp;
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fd = -1; //indicates a free fcb
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fd == -1)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
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
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;
	DE * currentFile;

        if (startup == 0) b_init();  //Initialize our system


	 if((flags | O_RDONLY)== O_RDONLY)
                {
                printf("passed O_RDONLY!\n");
                }
        if((flags & O_WRONLY)== O_WRONLY)
                {
                printf("passed O_WRONLY!\n");
                }

        if((flags & O_RDWR)== O_RDWR)
                {
                printf("passed O_WRONLY!\n");
                }

        if((flags & O_CREAT)== O_CREAT)
                {
                printf("passed O_CREAT!\n");
                }
        if((flags & O_TRUNC)== O_TRUNC)
                {
                printf("passed O_TRUNC!\n");
                }


        printf("b_open received filename: %s\n", filename);

	char * newPath= pathUpdate(filename);
	printf("newPath: %s\n", newPath);

	if(filename == NULL) return -1;

        parsePathInfo *ppi = malloc(sizeof(parsePathInfo));
	int pathValidity= parsePath(newPath, ppi);

	printf("pathValidity: %d, ppi->indexOfLastElement: %d\n", 
	pathValidity, ppi->indexOfLastElement);
    	// ppiTest->lastElement[0]= 'E';test for structure passing
    	// parsePath returns 0 if valid path for a directory, -2 if invalid

	//bad path
	if(pathValidity != 0)
		{
		printf("invalid pathname\n");
		 return -1;
		}
	//if found, cannot be directory
	if(ppi->indexOfLastElement != -1 && isDirectory(&ppi->parent[ppi->indexOfLastElement]))
		{
		printf("pathname is a directory.\n");
		return -1;
		}
	//if valid path, and not found, AND CREAT not specified, return -1
	if(ppi->indexOfLastElement == -1 && (flags & O_CREAT) != O_CREAT)
		{
		printf("file not found, no create permissions\n");
		return -1;
		}

	//at this point, only options left are create or load
	//get the file block to update it
        returnFd = b_getFCB();                          // get our own file descriptor
	if (returnFd== -1)
		{
		printf("All file descriptors in use!\n");
		return -1;
		}

	fcbArray[returnFd].fd=returnFd;
//	printf("file handle: %d\n", returnFd);

	//create a file
	if(ppi->indexOfLastElement == -1 && (flags & O_CREAT) == O_CREAT)
		{
		printf("condition to create a file!\n");
		
		char *empty = malloc(1);
    		empty[0] = '\0';
    		int nextAvailable = FindEntryInDir(ppi->parent, empty);
    		if (nextAvailable == -1)
    			{
        		printf("Directory at capacity.\n");
        		fcbArray[returnFd].fd= -1;
			return -1;
    			}
		free(empty);
		printf("nextAvailable in directory: %d\n", nextAvailable);

		currentFile= &(ppi->parent[nextAvailable]);

   		// update directory entry name for new file
    		int i = 0;
    		while (ppi->lastElement[i])
    			{
        		currentFile->fileName[i] = ppi->lastElement[i];
        		i++;
    			}
		printf("filename: |%s| added for dir[%d]\n", currentFile->fileName, nextAvailable);

		//update fcb
                fcbArray[returnFd].dirIndex= nextAvailable;
		}//end create section

	//load a file
	if(ppi->indexOfLastElement != -1)
		{
		printf("condition to load a file!\n");
		currentFile= &(ppi->parent[ppi->indexOfLastElement]);

		//update fcb
        	fcbArray[returnFd].dirIndex= ppi->indexOfLastElement;

		//if truncate, release all blocks inside extent tables and set filesize to 0
		if ((flags & O_TRUNC)== O_TRUNC)
			{
			printf("truncating file\n");
			EXTTABLE * ext= loadExtent(currentFile);
			//load extent new file
	                if(ext == NULL)
               		        {
                       		currentFile=NULL;
                       		ext=NULL;
                       		printf("loadExtent() error\n");
                       		fcbArray[returnFd].fd= -1;
                       		return -1;
                       		}

 			printf("extent for file says start is: %d\n", ext[ppi->indexOfLastElement].tableArray[0].start);
	                /* itr array of extents- max in system is 5 hardcoded,
               		but could do helper function that checks last extent for positive start and count of 0, 
			and continues itr in next extent table stored at start.
			*/
               		for(int i=0; i < 5; i++)
                        	{
       	               		ext[ppi->indexOfLastElement].tableArray[i].start= 0;
               	        	ext[ppi->indexOfLastElement].tableArray[i].count= 0;
                       		printf("ext array[%d] start: %d count: %d\n", i,  ext[ppi->indexOfLastElement].tableArray[i].start,
                       		ext[ppi->indexOfLastElement].tableArray[i].count);
                       		}

			}//end truncate
		}//end load file section


  //update fcb
                fcbArray[returnFd].permissions= flags;
                fcbArray[returnFd].parent= ppi->parent;
                fcbArray[returnFd].fileBuffer=malloc(B_CHUNK_SIZE*sizeof(char));
//              printf("fcb[fd].parent[nextavail].fileName: |%s|\n", fcbArray[returnFd].parent[nextAvailable].fileName);
//              printf("access test fcbArray[fd].parentExt[nextAvailable].tableArray[0].start : %d\n", 
//              fcbArray[returnFd].parentExt[nextAvailable].tableArray[0].start);


	b_close(returnFd);
	//return (returnFd);						// all set
	return -1;
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}


/* tested but not integrated here

        EXTTABLE * ext;

//load extent new file
                ext= loadExtent(currentFile);
                if(ext == NULL)
                        {
                        currentFile=NULL;
                        ext=NULL;
                        printf("loadExtent() error\n");
                        fcbArray[returnFd].fd= -1;
                        return -1;
                        }

 printf("extent for file says start is: %d\n", ext[nextAvailable].tableArray[0].start);
                 EXTENT *tempArray = allocateBlocks(START_ALLOCATION, START_ALLOCATION);
                 if(tempArray == NULL)
                        {
                        currentFile=NULL;
                        ext = NULL;
                        fcbArray[returnFd].fd= -1;
                        printf("allocateBlocks() error\n");
                        return -1;
                        }

                printf("allocation for new file begins at: %d\n", tempArray[0].start);
                        // fcb allocation =ALOCATE START
                //copy allocation info to extent for directory new file
                int tempArraySize= sizeof(tempArray) / sizeof(tempArray[0]);
                int itrMax = 5;
                if (tempArraySize < 5) itrMax= tempArraySize;
                /* copy to array of extents- max in system is 5 hardcoded,
                but could do a helper function that inits second extent table,
                stores location of new extent in start and count of 0, and continues
                copying to index of new extent table. Then when reading/writing,
                would check each extent array index for start >0 but count ==0 for next
                extent table to load.
                
                for(int i=0; i < itrMax; i++)
                        {
                        ext[nextAvailable].tableArray[i].start= tempArray[i].start;
                        ext[nextAvailable].tableArray[i].count= tempArray[i].count;
                        printf("ext array[%d] start: %d count: %d\n", i,  ext[nextAvailable].tableArray[i].start,
                        ext[nextAvailable].tableArray[i].count);
                        }

*/





		b_close(fd);
		//update filesize inside dir whenever allocate called.
	return (0); //Change this
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
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		b_close(fd);
	return (0);	//Change this
	}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
	{
//why is below free causing free problem?
//	if(fcbArray[fd].fileBuffer) free(fcbArray[fd].fileBuffer);
       fcbArray[fd].fd= -1;

//if write permissions for fcb
	//load extent for DE *
	// if allocated > START_ALLOCATION
		//release unused blocks by taking new fileSize in dir * in blocks and subtracting  (fileblocks?) member from structure with current block
	//write dir, //write extent

	}
