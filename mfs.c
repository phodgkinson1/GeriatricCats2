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
    char *newDir = malloc(256 * sizeof(char));
    strcpy(newDir, currentDir);
    printf("pathname 0 char : |%c|\n", pathname[0] != '/');
    if (currentDir[strlen(currentDir) - 1] != '/' && pathname[0] != '/')
    {
        strcat(newDir, "/");
    }
    strcat(newDir, pathname);
    printf("newdir pathname : |%s|\n", newDir);

    parsePathInfo *ppiTest = malloc(sizeof(parsePathInfo));
    //	ppiTest->lastElement[0]= 'E';

    // parsePath returns 0 if valid path for a directory, -2 if invalid
    int pathValidity = parsePath(newDir, ppiTest);
    //	printf("parsePath returned : %d\n", pathValidity);

    if (pathValidity == 0)
    {
        //		printf("ppi members- parent->filesize: %d, lastElement[0]: %s, indexOfLastElement: %d\n",
        //		ppiTest->parent->fileSize, ppiTest->lastElement, ppiTest->indexOfLastElement);
        char *empty = malloc(1);
        empty[0] = '\0';
        int nextAvailable = FindEntryInDir(ppiTest->parent, empty);
        if (nextAvailable == -1)
            return -1;
        //		printf("next avail: %d\n", nextAvailable);
        int startBlockNewDir = initDir(DEFAULT_ENTRIES, ppiTest->parent);
        //		printf("startBlockNewDir should be 31 at first call): %d \n", startBlockNewDir);
        // store in extent table for current directory entry in parent
        EXTTABLE *ext = loadExtent(ppiTest->parent);
        ext[nextAvailable].tableArray[0].start = startBlockNewDir;
        //		printf(" ext[nextAvailable].tableArray[0].start: %d\n",  ext[nextAvailable].tableArray[0].start);
        if(ext != NULL) free(ext);
        // update directory entry name for new directory
        char *copy = ppiTest->lastElement;
        int i = 0;
        while (copy[i])
        {
            ppiTest->parent[nextAvailable].fileName[i] = copy[i];
            i++;
        }
        //		printf("new filename at  ppiTest->parent[nextAvailable].fileName: |%s|\n",
        //		ppiTest->parent[nextAvailable].fileName);
        printf("Success- directory made\n");
    }
    else
    {
        printf("\n Invalid path!\n");
        return -1;
    }
    // cleanup
    if(ppiTest != NULL) free(ppiTest);
    return 0;
}



// Remove empty directory
int fs_rmdir(const char *pathname)
{
    // ParsePath
    parsePathInfo ppi;
    if (parsePath((char *)pathname, &ppi) != 0) return -1;

    // find index
    int index = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (index == -1) return -1;

    // load the directory to be removed
    DE *dirRemove = loadDir(ppi.parent, index);

    // check fs_isDir is 1 --> dir (must be return dir)
    if (fs_isDir((char *)pathname) != 1) return -1;

	//iterate through every entry in dirRemove and make sure empty. if files with name not '\0' then cannot remove return -1.

    // check loading directory is empty (must be empty)
    if (dirRemove != NULL) return -1;

    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(dirRemove);

    for (int i = 0; i < 5; i++)
    {
	if (extTable->tableArray[i].count > 0)
	{
	    releaseBlocks(extTable->tableArray[i].start, extTable->tableArray[i].count);
	}
    }

	//ext = loadExtent()
	//set ppi.parent[index] info in directory entry to "unused" 0/null/'\0' etc
	//call releaseBlocks (ext[index]->start, size???)

}

fdDir *fs_opendir(const char *pathname)
{

    printf("called fs_opendir with pathname : |%s|\n", pathname);

    // initilize a directory and a parsePathInfo struct
    DE *myDir = malloc(sizeof(DE)); // not sure if I need to malloc
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo)); 

    // Check for NULL pathname
    if (pathname == NULL)
    {
        printf("Directory Not Found");
        return NULL;
    }

    char *pathNameCopy = strdup(pathname);

    //call parsePath() to traverse and update ppiTest
    int parsePathCheck = parsePath(pathNameCopy, ppi);
    //printf("return value of parsePath(): %d",  parsePathCheck);

    //check if directory with pathname exists
    if(ppi->indexOfLastElement != -1){
        
        //check if pathname is a directory
        if(isDirectory(&ppi->parent[ppi->indexOfLastElement]) == 1){

            //load directory to initialize it in fdDir struct that will be the return value
            myDir = loadDir(ppi->parent, ppi->indexOfLastElement);
            fdDir *fdd = malloc(sizeof(fdDir));
            
            fdd->directory = myDir;
            fdd->dirEntryPosition = 0;
            fdd->d_reclen = sizeof(fdDir);

            return(fdd);
        }

        free(myDir);
        free(ppi);
    }
}

struct fs_diriteminfo *fs_readdir(fdDir *dirPath)
{
    // Read the next directory entry from the directory specified by dirPath
    // Update the dirEntryPosition in dirPath to keep track of the position
    // Return NULL if there are no more entries

    printf("Inside fs_readdir\n");

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
    /*
        strncpy(pathname, currentDir, size);
        return pathname;
    */
}

int fs_setcwd(char *pathname)
{
    // parsepath
    parsePathInfo ppi;
    int ppiResult = parsePath(pathname, &ppi);
    if (ppiResult !=0) return -1;

    // find index
    int index = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (index == -1) return -1;

    // must be directory
    if (fs_isDir(pathname) != 1) return -1;

    // free the prior current working directory
    free(cwd);

    // load the new directory
    cwd = loadDir(ppi.parent, index);

    char *newPath;
    // Absolute path
    if (pathname[0] == '/')
    {
         newPath = strdup(pathname);
    }
    // Relative path
    // Adjust /. and /..
    else 
    {
        // newPath = cwd + pathname

    }


}


// fs_isFIle and fs_isDir are similar
// The difference is  when file: return 0; when dir: return 1;
// This distinction is written in isDir(DE *dir) a helper function

int fs_isFile(char *filename)
{
    parsePathInfo ppi;
    int ppiResult = parsePath(filename, &ppi);
    if (ppiResult != 0) return -1;

    int index = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (index == -1) return -1;

    DE *dir = &(ppi.parent[index]);

    int isFileResult = isDirectory(dir);
    return isFileResult;
}


int fs_isDir(char *pathname)
{
    parsePathInfo ppi;
    int ppiResult = parsePath(pathname, &ppi);
    if (ppiResult != 0) return -1;

    int index = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (index == -1) return -1;

    DE *dir = &(ppi.parent[index]);
    int isDirResult = isDirectory(dir);

    return isDirResult;

}

int fs_delete(char *filename)
{
    // ParsePath
    parsePathInfo ppi;
    if (parsePath(filename, &ppi) != 0) return -1;

    // find index
    int index = FindEntryInDir(ppi.parent, ppi.lastElement);
    if (index == -1) return -1;

    // load the directory to be removed
    DE *dirRemove = loadDir(ppi.parent, index);

    // check fs_isFile is 0 --> file (must be return file)
    if (fs_isFile(filename) != 0) return -1;

    // Release the blokcs associated with dirRemove
    EXTTABLE *extTable = loadExtent(dirRemove);

    for (int i = 0; i < 5; i++)
    {
        if (extTable->tableArray[i].count > 0)
        {
            releaseBlocks(extTable->tableArray[i].start, extTable->tableArray[i].count);
        }
    }

    // Mark the directory as unused
    memset(dirRemove, 0, sizeof(DE));

    // Write to the disk (need to modify)
    //  LBAwrite();


}

// *********************** NEED TO CEHCK *******************************
// This would be used in "ls" and "touch" command?
// **** dont know how to check ****
int fs_stat(const char *path, struct fs_stat *buf) {

    // Start fs_stat
    printf("fs_stat : \n\n");

    // need to have 2 valid parameters in fs_stat()
    if (path == NULL || buf == NULL)
    {
        printf("parameters cannot be null\n"); // Fixed function name
        return -1;
    }

    // Copy the path into pathCopy
    char *pathCopy = strdup(path);
    if (pathCopy == NULL)
    {
        printf("Failed to duplicate path string\n");
        return -1;
    }

    // Allocate and zero-initialize ppi (parsePathInfo)
    parsePathInfo *ppi;
    memset(ppi, 0, sizeof(parsePathInfo));

    int pathResult = parsePath(pathCopy, ppi);
    free(pathCopy);


    DE *entry = NULL;

    if (ppi->indexOfLastElement >= 0 && ppi->parent != NULL) 
    {
        entry = &ppi->parent[ppi->indexOfLastElement];
    }

    buf->st_size = entry->fileSize;
    buf->st_blksize = BLOCK_SIZE;
    buf->st_blocks = (entry->fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    buf->st_accesstime = entry->lastAccessedTime;
    buf->st_modtime = entry->modifiedTime;
    buf->st_createtime = entry->createdTime;

    free(ppi->lastElement);

    return 0;
}
