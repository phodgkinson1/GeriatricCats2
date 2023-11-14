#include <sys/types.h>
#include "mfs.h"
#include "fsLow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mfsHelper.h"

// Structure to keep track of the current working directory
static char currentDir[256] = ""; // Initialize with root directory
// note! int cwdGlobal can be referenced from mfsHelper.h and must be updated with block number of start of current working directory

int fs_mkdir(const char *pathname, mode_t mode)
	{
    	// update pathname
    	printf("called fs_mkdr with pathname : |%s|\n", pathname);

	// update pathname with new element
    	char *newDir = malloc(256);
    	strcpy(newDir, currentDir);
    	printf("pathname 0 char : |%c|\n", pathname[0] != '/');
    	if (currentDir[strlen(currentDir) - 1] != '/' && pathname[0] != '/')
    		{
        	strcat(newDir, "/");
    		}
    	strcat(newDir, pathname);
    	printf("newdir pathname : |%s|\n", newDir);

    	parsePathInfo *ppiTest = malloc(sizeof(parsePathInfo));
    				//ppiTest->lastElement[0]= 'E';test for structure passing
    	// parsePath returns 0 if valid path for a directory, -2 if invalid
    	int pathValidity = parsePath(newDir, ppiTest);
    	//	printf("parsePath returned : %d\n", pathValidity);
	if(newDir !=NULL) free(newDir);

	if (pathValidity != 0)
    		{
		printf("Invalid path!\n");
		return -1;
		}

        //printf("ppi members- parent->filesize: %d, lastElement[0]: %s, indexOfLastElement: %d\n",
        //ppiTest->parent->fileSize, ppiTest->lastElement, ppiTest->indexOfLastElement);
        char *empty = malloc(1);
        empty[0] = '\0';
	int nextAvailable = FindEntryInDir(ppiTest->parent, empty);
        if (nextAvailable == -1)
		{
		printf("Directory at capacity.\n");
		return -1;
		}

	//load parent
        int startBlockNewDir = initDir(DEFAULT_ENTRIES, ppiTest->parent, nextAvailable);
 	printf("in mkdir startBlockNewDir: %d \n", startBlockNewDir);

        // load parent extent block
        EXTTABLE *ext = loadExtent(ppiTest->parent);

	//set parent's extent with start of new dir
        ext[nextAvailable].tableArray[0].start = startBlockNewDir;
	printf("ROOT PARENT ext[%d].tableArray[0].start: %d\n", nextAvailable, ext[nextAvailable].tableArray[0].start);
	int parentDirStart= ext[1].tableArray[0].start;
	printf("parent directory start block from extent table: %d\n", ext[1].tableArray[0].start);
	//write parent's extent
	writeExtent(ppiTest->parent, ext);
        if(ext != NULL) free(ext);

        // update directory entry name for new directory
        char *copy = ppiTest->lastElement;
        int i = 0;
        while (copy[i])
        	{
            	ppiTest->parent[nextAvailable].fileName[i] = copy[i];
            	i++;
        	}
	copy=NULL;
 	printf("new filename at  ppiTest->parent[nextAvailable].fileName: |%s|\n",
        ppiTest->parent[nextAvailable].fileName);

	printf("inside mk dir- parent[2] startextentblock: %d\n", ppiTest->parent[nextAvailable].extentBlockStart);
 	writeDir(ppiTest->parent, parentDirStart);

    	// cleanup
	if(ppiTest!=NULL)
		{
		free(ppiTest);
		ppiTest=NULL;
    		}

	return 1;
	}


// Remove empty directory
int fs_rmdir(const char *pathname)
{
    printf("fs_rmdir starts: \n");

    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));
    if (parsePath((char *)pathname, ppi) != 0) return -1;

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1) return -1;

    // check fs_isDir is 1 --> dir (must be return dir)
    if (fs_isDir((char *)pathname) != 1) return -1;

    // load the directory that targets to be removed
    DE *dirRemove = loadDir(ppi->parent, index);

    // by iterating through the entries, check its empty condition
    if (isDirEmpty(dirRemove) != 1) return -1;

    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(dirRemove);

    for (int i = 0; i < 5; i++)
    {
        if (extTable->tableArray[i].count > 0)
        {
            releaseBlocks(extTable->tableArray[i].start, extTable->tableArray[i].count);
        }
    }

    free(extTable);

    markDirUnused(dirRemove);
    writeDir(ppi->parent, index);

    free(dirRemove);
    free(ppi);
    return 0;
}


fdDir *fs_opendir(const char *pathname)
	{
    	printf("start of fs_opendir with pathname : |%s|\n", pathname);

    	// update pathname with new element
    	char *newDir = malloc(256 * sizeof(char));
    	strcpy(newDir, currentDir);
    	printf("pathname 0 char : |%c|\n", pathname[0] != '/');
    	if (currentDir[strlen(currentDir) - 1] != '/' && pathname[0] != '/')
    		{
        	strcat(newDir, "/");
    		}
    	strcat(newDir, pathname);
    	printf("newdir pathname : |%s|\n", newDir);

    	// initilize a directory and a parsePathInfo struct
    	DE *myDir;
    	parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    	// Check for NULL pathname
    	if (pathname == NULL)
    		{
        	printf("Directory Not Found");
        	return NULL;
    		}

    	//call parsePath() to traverse and update ppi
    	int parsePathCheck = parsePath(newDir, ppi);
    	printf("return value of parsePath(): %d\n",  parsePathCheck);

    	//check if directory with pathname exists
    	if(parsePathCheck != 0)
		{
		printf("Invalid path!\n");
		return NULL;
		}

        //check if pathname is a directory
	if(isDirectory(&ppi->parent[ppi->indexOfLastElement]) == 0)
		{
		printf("%s is not a directory\n", ppi->lastElement);
		return NULL;
		}

        //load directory to initialize it in fdDir struct that will be the return value
        myDir = loadDir(ppi->parent, ppi->indexOfLastElement);
        fdDir *fdd = malloc(sizeof(fdDir));

        fdd->directory = myDir;
    	fdd->dirEntryPosition = 0;
     	fdd->d_reclen = sizeof(fdDir);

   	printf("End of fs_opendir\n");
	//cleanup
	if(myDir) free(myDir);
	myDir=NULL;
    	if(ppi) free(ppi);
	ppi=NULL;
	
        return(fdd);
	}
//end of fs_opendir()


struct fs_diriteminfo *fs_readdir(fdDir *dirPath)
{
    // Read the next directory entry from the directory specified by dirPath
    // Update the dirEntryPosition in dirPath to keep track of the position
    // Return NULL if there are no more entries

    printf("Inside fs_readdir\n");
/*
    int directoryEntries = dirPath->directory[0].fileSize / sizeof(DE);
    
    //in the other section he used a for loop
    //how to know if a directory entry is being used? I used NULL
    while(&dirPath->directory[dirPath->dirEntryPosition] == NULL){
        dirPath->dirEntryPosition++;
        if(dirPath->dirEntryPosition >= directoryEntries){
            return NULL;
        }
    }

    //copy the name of currenct directory to fs_diriteminfo
    strcpy(dirPath->di->d_name, dirPath->directory[dirPath->dirEntryPosition].fileName);
    
    //check the type of the directory
    if(isDirectory(dirPath->directory) == 1){
        dirPath->di->fileType = 'd';
    }
    else{
        dirPath->di->fileType = 'f';
    }

    //update positon for next iteration
    dirPath->dirEntryPosition++;
    
    return dirPath->di;
	*/
	return NULL;
}

int fs_closedir(fdDir *dirPath)
{
    // Close the directory specified by dirPath
    releaseBlocks(dirPath->directory->extentBlockStart, dirPath->directory->extentIndex);

    // Free resources allocated for the dirPath structure
    free(dirPath);
    return 0;
}

char *fs_getcwd(char *pathname, size_t size) {

/*    printf("fs_getcwd function called\n");
    if (size <= 0 || pathname == NULL) {
        return NULL;
    }

    // Copy the current directory path into the provided buffer
    strncpy(pathname, currentDir, size);

    // Ensure the buffer is null-terminated
    pathname[size - 1] = '\0';
*/
    return pathname;
}


int fs_setcwd(char *pathname) {
    printf("fs_setcwd function called\n");
    if (pathname == NULL || strlen(pathname) == 0) {
        return -1; // Invalid path
    }

    // Tokenize the path
    char *token = strtok(pathname, "/");
    char tokens[256][256]; // 256 tokens with 256 characters each
    int numTokens = 0;

    while (token != NULL && numTokens < 256) {
        // Handle each token for '.' and '..'
        if (strcmp(token, ".") == 0) {
            // Current directory remains unchanged
        } else if (strcmp(token, "..") == 0) {
            // Remove the last component from currentDir (go up one directory)
            char *lastSlash = strrchr(currentDir, '/');
            if (lastSlash != NULL) {
                *lastSlash = '\0';
            }
        } else {
            // Normal case: add the token to the array
            strcpy(tokens[numTokens], token);
            numTokens++;
        }

        // Get the next token
        token = strtok(NULL, "/");
    }

    // Reconstruct the new path based on the tokens
    char newPath[256] = "";
    for (int i = 0; i < numTokens; i++) {
        strcat(newPath, "/");
        strcat(newPath, tokens[i]);
    }

    // Update the current directory path
    strncpy(currentDir, newPath, sizeof(currentDir));
    currentDir[sizeof(currentDir) - 1] = '\0';

    return 0; // Success
}

// fs_isFIle and fs_isDir are similar
// The difference is  when file: return 0; when dir: return 1;
// This distinction is written in isDir(DE *dir) a helper function

int fs_isFile(char *filename)
{
    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    if (parsePath(filename, ppi) != 0) {
        // Parsing failed, assuming not a file
        return 0;
    }

    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1) {
        // Entry not found, assuming not a file
        return 0;
    }

    DE *dirEntry = &(ppi->parent[index]);
    int result = dirEntry->isDirectory == 0; // Returns 1 if file (isDirectory == 0), 0 otherwise

    printf("result: %d \n", result);
    printf("File removed\n");

    free(ppi);
    return result;
}


//return 0 if file, returns 1 if dir
int fs_isDir(char *pathname)
{
    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    if (parsePath(pathname, ppi) != 0) {
        // Parsing failed, assuming not a directory
        return 0;
    }

    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1) {
        // Entry not found, assuming not a directory
        return 0;
    }

    DE *dirEntry = &(ppi->parent[index]);
    int result = dirEntry->isDirectory == 1; // Returns 1 if directory (isDirectory == 1), 0 otherwise

    printf("result: %d \n", result);
 //   printf("Dir removed\n");

    free(ppi);
    return result;
}



int fs_delete(char *filename)
{
    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));
    if (parsePath(filename, ppi) != 0) return -1;

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1) return -1;

    // load the directory to be removed
    // DE *dirRemove = loadDir(ppi.parent, index);

    // check fs_isFile is 0 --> file (must be return file)
    if (fs_isFile(filename) != 0) return -1;

    ppi->parent = &ppi->parent[index];
    DE *dirRemove = ppi->parent;

    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(dirRemove);

    for (int i = 0; i < 5; i++)
    {
        if (extTable->tableArray[i].count > 0)
        {
            releaseBlocks(extTable->tableArray[i].start, extTable->tableArray[i].count);
        }
    }


    // Write to the disk (need to modify)
    //  LBAwrite();

    free(ppi);
    return 0;

}

// *********************** NEED TO CEHCK *******************************
// This would be used in "ls" and "touch" command?
// **** dont know how to check ****
int fs_stat(const char *path, struct fs_stat *buf) {
    printf("fs_stat function called with path: %s\n", path);
    if (path == NULL || buf == NULL) {
        return -1; // Invalid input
    }

    // Parse the path to find the file or directory
    parsePathInfo ppi;
    if (parsePath((char*)path, &ppi) < 0) {
        return -1; // Parsing failed
    }

    // Check if the file or directory specified by 'path' exists
    int entryIndex = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (entryIndex < 0) {
        return -1; // File or directory does not exist
    }

    // Fill in 'buf' with file/directory statistics
    buf->st_size = 0; // Replace with actual file size
    buf->st_blksize = 512; // Blocksize for file system I/O (modify as needed)
    buf->st_blocks = 0; // Calculate the number of 512B blocks

    return 0; // Success
}

