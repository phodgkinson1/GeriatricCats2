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
    // ppiTest->lastElement[0]= 'E';test for structure passing
    // parsePath returns 0 if valid path for a directory, -2 if invalid
    int pathValidity = parsePath(newDir, ppiTest);
    //	printf("parsePath returned : %d\n", pathValidity);
    if (newDir != NULL)
        free(newDir);

    if (pathValidity != 0)
    {
        printf("Invalid path!\n");
        return -1;
    }

    // printf("ppi members- parent->filesize: %d, lastElement[0]: %s, indexOfLastElement: %d\n",
    // ppiTest->parent->fileSize, ppiTest->lastElement, ppiTest->indexOfLastElement);
    char *empty = malloc(1);
    empty[0] = '\0';
    int nextAvailable = FindEntryInDir(ppiTest->parent, empty);
    if (nextAvailable == -1)
    {
        printf("Directory at capacity.\n");
        return -1;
    }

    // load parent
    int startBlockNewDir = initDir(DEFAULT_ENTRIES, ppiTest->parent, nextAvailable);
    printf("in mkdir startBlockNewDir: %d \n", startBlockNewDir);

    // load parent extent block
    EXTTABLE *ext = loadExtent(ppiTest->parent);

    // set parent's extent with start of new dir
    ext[nextAvailable].tableArray[0].start = startBlockNewDir;
    printf("ROOT PARENT ext[%d].tableArray[0].start: %d\n", nextAvailable, ext[nextAvailable].tableArray[0].start);
    int parentDirStart = ext[1].tableArray[0].start;
    printf("parent directory start block from extent table: %d\n", ext[1].tableArray[0].start);
    // write parent's extent
    writeExtent(ppiTest->parent, ext);
    if (ext != NULL)
        free(ext);

    // update directory entry name for new directory
    char *copy = ppiTest->lastElement;
    int i = 0;
    while (copy[i])
    {
        ppiTest->parent[nextAvailable].fileName[i] = copy[i];
        i++;
    }
    copy = NULL;
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


// Remove empty directory ***** (DONE)
int fs_rmdir(const char *pathname)
{
// -----------------------------------------------------------------------------
// This will be created as a helper function somewhere
    printf("fs_rmdir starts: \n");
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

// -----------------------------------------------------------------------------


    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    int parsePathResult = parsePath((char *)pathname, ppi);
    printf("parsePathResult : %d \n", parsePathResult);

    if (parsePathResult != 0) return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1) return -1;  // find index check done(o)

    // check fs_isDir is 1 --> dir (must be return dir)
    int checkDir = fs_isDir((char *)pathname);
    printf("fs_isDir result: %d \n", checkDir); 

    if (checkDir != 1) return -1; // check fs_isDir check done(o)

    // load the directory that targets to be removed
    DE *dirRemove = &ppi->parent[index];

    printf("Original dirRemove: \n");
    printf("fileName: %s \n", dirRemove->fileName);
    printf("fileSize: %d \n", dirRemove->fileSize);
    printf("isDirectory: %d \n", dirRemove->isDirectory);

    // by iterating through the entries, check its empty condition
    // checkDirEmpty == 0 -----> target dir is empty ----> ready to remove
    int checkDirEmpty = isDirEmpty(dirRemove);

    if (checkDirEmpty != 0) 
    {
        free(dirRemove);
        return -1;
    }
    printf("check Dir Empty: %d \n", checkDirEmpty);
    printf("0 means ready to remove the dir!!\n");


    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(dirRemove);

    for (int i = 0; i < 5; i++)
    {
        if (extTable[index].tableArray[i].start > 0)
        {
            releaseBlocks(extTable[index].tableArray[i].start, extTable[index].tableArray[i].count);
        }
    }

    markDirUnused(dirRemove);

    writeDir(ppi->parent, index);

    printf("Updated dirRemove: \n");
    printf("fileName: %s \n", dirRemove->fileName);
    printf("fileSize: %d \n", dirRemove->fileSize);
    printf("isDirectory: %d \n", dirRemove->isDirectory);

    free(ppi);
    return 0;
}

fdDir *fs_opendir(const char *pathname)
{
    printf("\nstart of fs_opendir with pathname : |%s|\n", pathname);

    if (pathname == NULL)
    {
        printf("Directory Not Found");
        return NULL;
    }

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

    // call parsePath() to traverse and update ppi
    int parsePathCheck = parsePath(newDir, ppi);
    printf("Inside fs_opendir return value of parsePath(): %d\n", parsePathCheck);
    printf("Inside fs_opendir ppi indexOfLast Element: %d\n", ppi->indexOfLastElement);

    // check if directory with pathname exists
    if (parsePathCheck != 0)
    {
        printf("Invalid path!\n");
        return NULL;
    }

    // check if pathname is a directory

    if (ppi->indexOfLastElement < 0)
    {
        printf("%s is not a directory\n", ppi->lastElement);
        return NULL;
    }

    printf("Inside of fs_opendir isDirectory() value returned: %d\n", isDirectory(&ppi->parent[ppi->indexOfLastElement]));
    printf("Inside of fs_opendir ppi parent isDirectory value: %d\n", ppi->parent[ppi->indexOfLastElement].isDirectory);

    if (isDirectory(&ppi->parent[ppi->indexOfLastElement]) <= 0)
    {
        printf("%s is not a directory\n", ppi->lastElement);
        return NULL;
    }

    // load directory to initialize it in fdDir struct that will be the return value
    myDir = loadDir(ppi->parent, ppi->indexOfLastElement);
    fdDir *fdd = malloc(sizeof(fdDir));

    fdd->directory = myDir;
    printf("openDir fdd->firectory.isDirectory: %d\n", fdd->directory->isDirectory);
    fdd->dirEntryPosition = 0;
    fdd->d_reclen = sizeof(fdDir);

   	printf("End of fs_opendir\n");
	//cleanup
	if(myDir) free(myDir);
	if(ppi) free(ppi);

        return(fdd);
	}
//end of fs_opendir()


struct fs_diriteminfo *fs_readdir(fdDir *dirPath)
{
    // Read the next directory entry from the directory specified by dirPath
    // Update the dirEntryPosition in dirPath to keep track of the position
    // Return NULL if there are no more entries

    printf("\nInside fs_readdir\n");
    //int directoryEntries = dirPath->directory->fileSize / sizeof(DE);
    int directoryEntries = 0;
    printf("fs_readdir directoryEntries value: %d\n", directoryEntries);

    // in the other section he used a for loop
    // how to know if a directory entry is being used? I used NULL
    while (dirPath->directory[dirPath->dirEntryPosition].fileName[0]== '\0')
    {
        dirPath->dirEntryPosition++;
        if (dirPath->dirEntryPosition >= directoryEntries)
        {
            return NULL;
        }
    }

    // copy the name of currenct directory to fs_diriteminfo
    strcpy(dirPath->di->d_name, dirPath->directory[dirPath->dirEntryPosition].fileName);
    printf("di->d_name: %s\n", dirPath->di->d_name);

    // check the type of the directory
    if (isDirectory(dirPath->directory) == 1)
    {
        dirPath->di->fileType = 'd';
    }
    else
    {
        dirPath->di->fileType = 'f';
    }

    // update positon for next iteration
    dirPath->dirEntryPosition++;

    return dirPath->di;
    
}

int fs_closedir(fdDir *dirPath)
{
    // Close the directory specified by dirPath
    releaseBlocks(dirPath->directory->extentBlockStart, dirPath->directory->extentIndex);

    // Free resources allocated for the dirPath structure
    free(dirPath);
    return 0;
}

char *fs_getcwd(char *pathname, size_t size)
{

    
    return pathname;
}


int fs_setcwd(char *pathname) {

    printf("fs_setcwd starts: \n");

    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    int parsePathResult = parsePath(pathname, ppi);
    printf("parsePathResult : %d \n", parsePathResult);

    if (parsePathResult != 0) return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1) return -1;  // find index check done(o)

    // Check if the target is a directory
    if (!isDirectory(&ppi->parent[ppi->indexOfLastElement])) {
        return -1; // Target is not a directory
    }

    // Free the previous cwd if it is not the root directory
    if (cwd != rootDir) free(cwd);

    cwd = loadDir(ppi->parent, index);

    char *pathComponents[32];
    int componentCount = 0;
    cwdGlobal = index;
    printf("cwdGlobal before iterating through: %d \n", cwdGlobal);
// -----------------------------------------------------------------

    // Absolute path, which always start from /
    if (pathname[0] == '/') 
    {
	absolutePath = pathname;
    }
    // Relative path  is affected by current working directory
    else
    {
        // first tokenize and get them as an array
	char *token = strtok(pathname, "/");
        while (token != NULL && componentCount < 32)
        {
	    pathComponents[componentCount++] = strdup(token);
	    token = strtok(NULL, "/");

	    printf("Tokenized pathname : %s \n", pathComponents[componentCount++]);
        }

	for (int i = 0; i < componentCount; i++)
        {
	    if (pathComponents[i] == ".")
	    {
		continue;
	    }
	    else if (pathComponents[i] == "..")
	    {
		cwdGlobal--;
		cwd = loadDir(rootDir, cwdGlobal);

	    }
	    else
	    {
		// THis should deal with regular like Dcouments/cat/dang/shit
            }
	}
    }



    return 0; // Success
}

// fs_isFIle and fs_isDir are similar
// The difference is  when file: return 0; when dir: return 1;
// This distinction is written in isDir(DE *dir) a helper function

int fs_isFile(char *filename)
{
    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    if (parsePath(filename, ppi) != 0)
    {
        // Parsing failed, assuming not a file
        return 0;
    }

    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1) 
    {
        // Entry not found, assuming not a file
        return 0;
    }

    DE *dirEntry = &(ppi->parent[index]);
    int result = dirEntry->isDirectory == 0; // Returns 1 if file (isDirectory == 0), 0 otherwise


    free(ppi);
    return result;
}

// return 0 if file, returns 1 if dir
int fs_isDir(char *pathname)
{
    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    if (parsePath(pathname, ppi) != 0)
    {
        // Parsing failed, assuming not a directory
        return 0;
    }

    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1)  
    {
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


// delete file (same concept with fs_rmDir)
int fs_delete(char *filename)
{
    printf("fs_delete starts: \n");

    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    int parsePathResult = parsePath(filename, ppi);
    printf("parsePathResult : %d \n", parsePathResult);

    if (parsePathResult != 0) return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1) return -1;  // find index check done(o)

    // check fs_isDir is 1 --> dir (must be return dir)
    int checkFile = fs_isFile(filename);
    printf("fs_isDir result: %d \n", checkFile); 

    if (checkFile != 1) return -1; // check fs_isDir check done(o)

    // load the directory that targets to be removed
    DE *fileRemove = &ppi->parent[index];

    printf("Original File: \n");
    printf("fileName: %s \n", fileRemove->fileName);
    printf("fileSize: %d \n", fileRemove->fileSize);
    printf("isDirectory: %d \n", fileRemove->isDirectory);



    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(fileRemove);

    for (int i = 0; i < 5; i++)
    {
        if (extTable[index].tableArray[i].start > 0)
        {
            releaseBlocks(extTable[index].tableArray[i].start, extTable[index].tableArray[i].count);
        }
    }

    markDirUnused(fileRemove);

    writeDir(ppi->parent, index);

    printf("Updated File Remove: \n");
    printf("fileName: %s \n", fileRemove->fileName);
    printf("fileSize: %d \n", fileRemove->fileSize);
    printf("isDirectory: %d \n", fileRemove->isDirectory);

    free(ppi);
    return 0;
}

// *********************** NEED TO CEHCK *******************************
// This would be used in "ls" and "touch" command?
// **** dont know how to check ****
int fs_stat(const char *path, struct fs_stat *buf)
{
    printf("fs_stat function called with path: %s\n", path);
    if (path == NULL || buf == NULL)
    {
        return -1; // Invalid input
    }

    // Parse the path to find the file or directory
    parsePathInfo ppi;
    if (parsePath((char *)path, &ppi) < 0)
    {
        return -1; // Parsing failed
    }

    // Check if the file or directory specified by 'path' exists
    int entryIndex = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (entryIndex < 0)
    {
        return -1; // File or directory does not exist
    }

    // Fill in 'buf' with file/directory statistics
    buf->st_size = 0;      // Replace with actual file size
    buf->st_blksize = 512; // Blocksize for file system I/O (modify as needed)
    buf->st_blocks = 0;    // Calculate the number of 512B blocks

    return 0; // Success
}


