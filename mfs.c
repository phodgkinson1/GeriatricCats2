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
        free(ext);
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
    free(ppiTest);
    return 0;
}

// someone's notes for below

/*

opendir (path)
ParsePath (path, ppi)
if (index != -1)
{
        isDirectory (&(ppi->parent[ppi->index]))
        myDir = loadDir (&(ppi->parent[ppi->index]))
        fdDir *fdd = malloc(sizeof(fdDir))

        fdd->directory = myDir
        fdd->directoryPos = 0;
        fdd->recLen = sizeof(fdDir);

        return (fdd);
}

readdir (fdDir *fdd)
{
        while ((fdd->directory[fdd->directoryPos]) not used)
        {
                ++ directoryPos
                if (directoryPos >= lastpos) return NULL;

                strcpy(fdd->di.name, fdd->directory[fdd->directoryPos].name)
                fdd->di.type = file || directory

                ++ directoryPos?? again??

                return di
        }
}



*/

int fs_rmdir(const char *pathname)
{
    /*   char path[256];
       strcpy(path, currentDir);
       if (currentDir[strlen(currentDir) - 1] != '/') {
           strcat(path, "/");
       }
       strcat(path, pathname);

       if (!fs_isDir(path)) {
           // Directory does not exist
           return -1;
       }

       // Remove the directory
       // Implementation specific, use fsLow.h functions
       // Update your directory structure?
   */
    return 0;
}

fdDir *fs_opendir(const char *pathname)
{

    printf("called fs_opendir with pathname : |%s|\n", pathname);

    // initilize a directory and a parsePathInfo struct
    DE *myDir = malloc(sizeof(DE)); // not sure if I need to malloc
    parsePathInfo *ppiTest = malloc(sizeof(parsePathInfo)); 

    // Check for NULL pathname
    if (pathname == NULL)
    {
        printf("Directory Not Found");
        return NULL;
    }

    //call parsePath() to traverse and update ppiTest
    int parsePathCheck = parsePath(pathname, ppiTest);
    //printf("return value of parsePath(): %d",  parsePathCheck);

    //check if directory with pathname exists
    if(ppiTest->indexOfLastElement != -1){
        
        //check if pathname is a directory
        if(isDir(&ppiTest->parent[ppiTest->indexOfLastElement])){

            //load directory to initialize it in fdDir struct that will be the return value
            myDir = loadDir(&ppiTest->parent, ppiTest->indexOfLastElement);
            fdDir *fdd = malloc(sizeof(fdDir));
            
            fdd->directory = myDir;
            fdd->dirEntryPosition = 0;
            fdd->d_reclen = sizeof(fdDir);

            return(fdd);
        }
    }
}

struct fs_diriteminfo *fs_readdir(fdDir *dirPath)
{
    // Read the next directory entry from the directory specified by dirPath
    // Update the dirEntryPosition in dirPath to keep track of the position
    // Return NULL if there are no more entries

    // Implementation specific

    return NULL;
}

int fs_closedir(fdDir *dirPath)
{
    // Close the directory specified by dirPath
    // Free resources allocated for the dirPath structure

    // Implementation specific

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
    // Set the current working directory

    // Implementation specific
    /*
        strcpy(currentDir, pathname);
        return 0;
    */
}

int fs_isFile(char *filename)
{
    // Check if the given filename is a regular file
    // Implementation specific
    // Use fsLow.h functions to check the file type

    // if (LBAread(...)) {
    //     return 1; // It's a file
    // } else {
    //     return 0; // It's not a file
    // }
    return 0;
}

int fs_isDir(char *pathname)
{

    // if (LBAread(...)) {
    //     return 1; // It's a directory
    // } else {
    //     return 0; // It's not a directory
    // }
    return 0;
}

int fs_delete(char *filename)
{

    // if (LBAwrite(...)) {
    //     return 0; // Deleted successfully
    // } else {
    //     return -1; // Deletion failed
    // }
    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf)
{

    // if (LBAread(...)) {
    //     // Fill in buf with file statistics
    //     return 0; // Success
    // } else {
    //     return -1; // Error
    // }
    return 0;
}
