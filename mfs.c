#include <sys/types.h>
#include "mfs.h"
#include "fsLow.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mfsHelper.h"

int fs_mkdir(const char *pathname, mode_t mode)
{
    // update pathname
    printf("called fs_mkdr with pathname : |%s|\n", pathname);
    if (pathname == NULL)
        return -1;
    // ******* add condition here to check if cdw==rootDir if no absolute
    // update pathname with new element
    char *newPath = pathUpdate(pathname);

    parsePathInfo *ppiTest = malloc(sizeof(parsePathInfo));
    // ppiTest->lastElement[0]= 'E';test for structure passing
    // parsePath returns 0 if valid path for a directory, -2 if invalid
    int pathValidity = parsePath(newPath, ppiTest);
    //	printf("parsePath returned : %d\n", pathValidity);
    if (newPath != NULL)
        free(newPath);

    if (pathValidity != 0)
    {
        printf("Invalid path!\n");
        return -1;
    }
    if (ppiTest->indexOfLastElement != -1)
    {
        printf("already a directory with same name.\n");
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
	free(empty);
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

    //    printf("new filename at  ppiTest->parent[nextAvailable].fileName: |%s|\n",
    //           ppiTest->parent[nextAvailable].fileName);

    printf("inside mk dir- parent[2] startextentblock: %d\n", ppiTest->parent[nextAvailable].extentBlockStart);

    writeDir(ppiTest->parent, parentDirStart);

    writeDir(loadDir(ppiTest->parent, nextAvailable), startBlockNewDir);

    if (ppiTest->parent[1].extentBlockStart == cwd[1].extentBlockStart)
    {
        cwd = ppiTest->parent;
    }

    // cleanup
    if (ppiTest != NULL)
    {
	if(ppiTest->parent != rootDir && ppiTest->parent != cwd)
		{
		free(ppiTest->parent);
		}
        free(ppiTest);
        ppiTest = NULL;
    }

    return 1;
}

// Remove empty directory ***** (DONE)
int fs_rmdir(const char *pathname)
{
    // -----------------------------------------------------------------------------
    printf("fs_rmdir starts: \n");
    printf("called fs_mkdr with pathname : |%s|\n", pathname);

    if (pathname == NULL)
        return -1;

    // update pathname with new element
      char *newPath = pathUpdate(pathname);

    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    int parsePathResult = parsePath(newPath, ppi);
    printf("parsePathResult : %d \n", parsePathResult);
    if (newPath != NULL)
        free(newPath);

    if (parsePathResult != 0)
        return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1)
        return -1; // find index check done(o)

    // check fs_isDir is 1 --> dir (must be return dir)
    int checkDir = fs_isDir((char *)pathname);
    printf("fs_isDir result: %d \n", checkDir);

    if (checkDir != 1)
        return -1; // check fs_isDir check done(o)

    // load the directory that targets to be removed
    DE *dirRemove = &ppi->parent[index];

    printf("Original dirRemove: \n");
    printf("fileName: %s \n", dirRemove->fileName);
    printf("fileSize: %d \n", dirRemove->fileSize);
    printf("isDirectory: %d \n", dirRemove->isDirectory);

    // by iterating through the entries, check its empty condition
    // checkDirEmpty == 0 -----> target dir is empty ----> ready to remove
    int checkDirEmpty = isDirEmpty(dirRemove);

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

	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
	{
        free(ppi);
	ppi=NULL;
	}
 
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
    char *newPath = pathUpdate(pathname);

    // initilize a directory and a parsePathInfo struct
    DE *myDir;
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    // call parsePath() to traverse and update ppi
    int parsePathCheck = parsePath(newPath, ppi);
    printf("Inside fs_opendir return value of parsePath(): %d\n", parsePathCheck);
    printf("Inside fs_opendir ppi indexOfLast Element: %d\n", ppi->indexOfLastElement);
    if (newPath != NULL)
        free(newPath);

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
    printf("Inside of fs_opendir ppi->indexOfLastElement: %d\n", ppi->indexOfLastElement);

    fdDir *fdd = malloc(sizeof(fdDir));
	
    fdd->directory = myDir;
    printf("openDir fdd->firectory.isDirectory: %d\n", fdd->directory->isDirectory);
    fdd->dirEntryPosition = 0;
    fdd->d_reclen = sizeof(fdDir);

    printf("End of fs_opendir\n");
    // cleanup


	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi!= NULL) free(ppi);

    return (fdd);
}
// end of fs_opendir()

struct fs_diriteminfo *fs_readdir(fdDir *dirPath)
{
    // Read the next directory entry from the directory specified by dirPath
    // Update the dirEntryPosition in dirPath to keep track of the position
    // Return NULL if there are no more entries

    printf("\nInside fs_readdir\n");
    int directoryEntries = dirPath->directory->fileSize / sizeof(DE);
    // int directoryEntries = 0;
    printf("fs_readdir directoryEntries value: %d\n", directoryEntries);
    printf("dirPath->dirEntryPosition: %d\n", dirPath->dirEntryPosition);

    if (dirPath == NULL || dirPath->directory == NULL)
    {
        printf("dirPath is NULL\n");
        return NULL;
    }

    printf("After checking if dirPath = NULL\n");

    // check if the directory entry is being used, iterate until you find a use entry
    while (dirPath->directory[dirPath->dirEntryPosition].fileName[0] == '\0')
    {
        printf("Inside while loop\n");
        dirPath->dirEntryPosition++;
        if (dirPath->dirEntryPosition >= directoryEntries)
        {
            return NULL;
        }
    }

	struct fs_diriteminfo * dii= malloc(sizeof(struct fs_diriteminfo));
	dirPath->di= dii;

    // copy the name of currenct directory to fs_diriteminfo
    strcpy(dirPath->di->d_name, dirPath->directory[dirPath->dirEntryPosition].fileName);
    printf("di->d_name: %s\n", dirPath->di->d_name);

    // check the type of the directory
    if (isDirectory(&dirPath->directory[dirPath->dirEntryPosition]) == 1)
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
    printf("fs_getcwd function called\n");
    if (size <= 0 || pathname == NULL)
    {
        return NULL;
    }

    // to check
    // long cwd = 4096;

    if (strlen(cwdAbsolutePath) > size - 1)
    {
        printf("buffer size is not fit into cwdAbsolutePath");
        return NULL;
    }

    printf("cwdAbsolutePath's length: %ld \n", strlen(cwdAbsolutePath));
    printf("Limitation of buffer size: %ld \n", size - 1);

    strncpy(pathname, cwdAbsolutePath, size);

    pathname[size - 1] = '\0';

    printf("fs_getcwd: %s \n", pathname);

    return pathname;
}

int fs_setcwd(char *pathname)
{
	if(rootDir==NULL) loadDir(rootDir, rootGlobal);
    printf("fs_setcwd starts with cwdAbsolutePath: %s\n", cwdAbsolutePath);

    printf("setcwd start root[0]:|%s| filesize: %d _____ root[1]: |%s| filesize: %d  root[2]: |%s| filesize: %d\n", rootDir[0].fileName,
           rootDir[0].fileSize, rootDir[1].fileName, rootDir[1].fileSize, rootDir[2].fileName, rootDir[2].fileSize);

    printf("setcwd start cwd[0]:|%s| filesize: %d _____ cwd[1]: |%s| filesize: %d  cwd[2]: |%s| filesize: %d\n", cwd[0].fileName,
           cwd[0].fileSize, cwd[1].fileName, cwd[1].fileSize, cwd[2].fileName, cwd[2].fileSize);

    char *pathComponents[32];
    int componentCount = 0;
    char *newPath = malloc(256);

    // ParsePath
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));

    if (pathname[0] == '/')
        strcat(newPath, "/");

    // first tokenize path components to array and parse '.' and '..'
    char *saveptr = NULL;
    char *token = strtok_r(pathname, " /", &saveptr);

    if (token != NULL)
    {
        pathComponents[componentCount] = strdup(token);
        printf("token %d: |%s| pathComponents[%d]: |%s|\n", componentCount, token, componentCount, pathComponents[componentCount]);
        componentCount++;
    }
	if (strcmp(token, "..") == 0)
		 {
                        //get second to last item from parsePath
                        int parsePathResult0 = parsePath(cwdAbsolutePath, ppi);
                        if(parsePathResult0 == 0)
                                {
                                        printf("parsePathResult set to parent");
                                        cwd=ppi->parent;
                                         printf("setcwd after cd . root[0]:|%s| filesize: %d _____ root[1]: |%s| filesize: %d  root[2]: |%s| filesize: %d\n", rootDir[0].fileName,
                                        rootDir[0].fileSize, rootDir[1].fileName, rootDir[1].fileSize, rootDir[2].fileName, rootDir[2].fileSize);

                                        printf("setcwd after cd . cwd[0]:|%s| filesize: %d _____ cwd[1]: |%s| filesize: %d  cwd[2]: |%s| filesize: %d\n", cwd[0].fileName,
                                        cwd[0].fileSize, cwd[1].fileName, cwd[1].fileSize, cwd[2].fileName, cwd[2].fileSize);

                                        printf("cwdAbsolutePath before cd .. : |%s|\n", cwdAbsolutePath);
                                        printf("strlen cwdABspath: |%ld|\n", strlen(cwdAbsolutePath));
                                	int i= strlen(cwdAbsolutePath)-1;
					while(cwdAbsolutePath[i]!='/'){
					i--;
					}
					i++;
					cwdAbsolutePath[i]= '\0';
					printf("new cwdAbsolutePath after cd . : |%s|\n", cwdAbsolutePath);
					componentCount--; 
				 	pathComponents[componentCount]= "";
				}
                         }
	if (strcmp(token, ".") == 0)
		{
		componentCount--;
                pathComponents[componentCount]= "";
		}


    while (token != NULL && componentCount < 32)
    {
        token = strtok_r(NULL, " /", &saveptr);
        if (token == NULL) break;

	if(strcmp(token, ".") == 0)
        	{
		if (componentCount > 0)
			{
			componentCount--;
			pathComponents[componentCount]= "";
			}
		}
        else if (strcmp(token, "..") != 0 )
        {
            // Normal directory component
            pathComponents[componentCount] = strdup(token);
            componentCount++;
        }
    }

	if(componentCount==0){
		return 0;

		}
    printf("value of cwdAbsolutepath: |%s|\n", cwdAbsolutePath);

    // add cwd to path (Relative)
    if (pathname[0] != '/')
    {
        strcpy(newPath, cwdAbsolutePath);
        // Ensure there's a trailing slash after cwdAbsolutePath, if not already present
        if (cwdAbsolutePath[strlen(cwdAbsolutePath) - 1] != '/')
        {
            strcat(newPath, "/");
        }
    }

    for (int i = 0; i < componentCount; i++)
    {
        strcat(newPath, pathComponents[i]);
        if (i != componentCount - 1)
            strcat(newPath, "/");
    }

    // ParsePath
    int parsePathResult = parsePath(pathname, ppi);

    if (parsePathResult != 0)
        return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1)
    {
        printf("find entry returned not found\n");
        return -1; // find index check done(o)
    }

    // Check if the target is a directory
    if (isDirectory(&ppi->parent[ppi->indexOfLastElement]) == 0)
    {
        printf("%s is not a directory \n", ppi->lastElement);
        return -1; // Target is not a directory
    }

    if (cwdAbsolutePath != NULL)
        free(cwdAbsolutePath);
    cwdAbsolutePath = newPath;
    printf("stored in cwdAbsolutePath: |%s|\n", cwdAbsolutePath);

    // set new cwd, cannot free old
	if(cwd != rootDir) free(cwd);
    cwd = loadDir(ppi->parent, index);

    printf("2 setcwd root[0]:|%s| filesize: %d _____ root[1]: |%s| filesize: %d  root[2]: |%s| filesize: %d\n", rootDir[0].fileName,
           rootDir[0].fileSize, rootDir[1].fileName, rootDir[1].fileSize, rootDir[2].fileName, rootDir[2].fileSize);

    printf("2 setcwd cwd[0]:|%s| filesize: %d _____ cwd[1]: |%s| filesize: %d  cwd[2]: |%s| filesize: %d\n", cwd[0].fileName,
           cwd[0].fileSize, cwd[1].fileName, cwd[1].fileSize, cwd[2].fileName, cwd[2].fileSize);

    // cleanup
	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
        {
        free(ppi);
        ppi=NULL;
        }

    return 0; // Success
}



// fs_isFIle and fs_isDir are similar
// The difference is  when file: return 0; when dir: return 1;
// This distinction is written in isDir(DE *dir) a helper function

int fs_isFile(char *filename)
{
    if (filename == NULL)
    {
        printf("invalid file name\n");
        return -1;
    }

    // update pathname with new element
    char *newPath = pathUpdate(filename);

    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    if (parsePath(filename, ppi) != 0)
        return 0;

    if (newPath != NULL)
        free(newPath);

    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index == -1)
    {
        // Entry not found, assuming not a file
        return 0;
    }

    DE *dirEntry = &(ppi->parent[index]);
    int result = dirEntry->isDirectory == 0; // Returns 1 if file (isDirectory == 0), 0 otherwise

	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
        {
        free(ppi);
        ppi=NULL;
        }

    return result;
}

// return 0 if file, returns 1 if dir
int fs_isDir(char *pathname)
{
    if (pathname == NULL)
    {
        printf("invalid pathname\n");
        return -1;
    }

    // update pathname with new element
    char *newPath = pathUpdate(pathname);

    parsePathInfo *ppi;
    ppi = malloc(sizeof(parsePathInfo)); // Allocate and initialize ppi

    printf("isDir before parsepath call\n");

    if (parsePath(pathname, ppi) != 0)
    {
        // Parsing failed, assuming not a directory
        return 0;
    }
    if (newPath != NULL)
        free(newPath);

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

if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
        {
        free(ppi);
        ppi=NULL;
        }

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

    if (parsePathResult != 0)
        return -1; // ParsePath check done(o)

    // find index
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    printf("index : %d \n", index);

    if (index == -1)
        return -1; // find index check done(o)

    // check fs_isDir is 1 --> dir (must be return dir)
    int checkFile = fs_isFile(filename);
    printf("fs_isDir result: %d \n", checkFile);

    if (checkFile != 1)
        return -1; // check fs_isDir check done(o)

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

	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
        {
        free(ppi);
        ppi=NULL;
        }

    return 0;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
    printf("fs_stat function called with path: %s\n", path);
    if (path == NULL || buf == NULL)
    {
        return -1; // Invalid input
    }

    // Parse the path to find the file or directory
    parsePathInfo *ppi = malloc(sizeof(parsePathInfo));
    int parsePathResult = parsePath((char *)path, ppi);

    if (parsePathResult != 0)
    {
        printf("PARSE PATH FAILED");
        return -1; // Parsing failed
    }

    // Check if the file or directory specified by 'path' exists
    int index = FindEntryInDir(ppi->parent, ppi->lastElement);
    if (index < 0)
    {
        printf("INDEX RETRIEVE FAILED");
        return -1; // File or directory does not exist
    }

    // fill in dir->information into buf->information

    DE *dir = &(ppi->parent[index]);

    buf->st_size = dir->fileSize;
    buf->st_blksize = BLOCK_SIZE;
    buf->st_blocks = (dir->fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    buf->st_accesstime = dir->lastAccessedTime;
    buf->st_modtime = dir->modifiedTime;
    buf->st_createtime = dir->createdTime;
    buf->st_isdir = dir->isDirectory;

	if(ppi->parent != rootDir && ppi->parent != cwd)
                {
                free(ppi->parent);
                }

    if (ppi != NULL)
        {
        free(ppi);
        ppi=NULL;
        }

    return 0; // Success
}

// cmd: move

int fs_move(char *fileName, char* destinationDir)
{
	if (fileName == NULL || destinationDir == NULL)
	{
		printf("Parameters are NULL\n");
	}

	// Parse the fileName **
	parsePathInfo *ppiF = malloc(sizeof(parsePathInfo));

	int parsePathResultF = parsePath(fileName, ppiF);
	if (parsePathResultF != 0)
	{
		printf("ParsePathResult F is invalid\n");
		return -1;
	}

	// check fileName is file
	int isFileResult = fs_isFile(fileName);
	if (isFileResult != 1)
	{
		printf("isFileResult is not 1, which means not a file\n");
		return -1;
	}

	int indexF = FindEntryInDir(ppiF->parent, ppiF->lastElement);
	if (indexF == -1) return -1;

	// ***** 1) parsePath fileName 2) check whether fileName is file 3) find the index

	// ******************************************************************************

	parsePathInfo *ppiD = malloc(sizeof(parsePathInfo));

	int parsePathResultD = parsePath(destinationDir, ppiD);
	if (parsePathResultD != 0)
	{
		printf("ParsePathResult2 is invalid\n");
		return -1;
	}

	int isDirResult = fs_isDir(destinationDir);
	if (isDirResult != 1)
	{
		printf("isDirResult is not 1, which means not a directory");
		return -1;
	}

	int properIndexD = FindEntryInDir(ppiD->parent, "");
	if (properIndexD == -1)
	{
		printf("Destination directory is usable\n");
		return -1;
	}

        // ***** 1) parsePath destinationDir 2)check whether it is dir 3) find the index

	// ------- Add file to destination directory
	DE *originalFile = &ppiF->parent[indexF];
	DE *newDir = &ppiD->parent[properIndexD];

	// Copy original meta data into newDir
	strncpy(newDir->fileName, originalFile->fileName, DIR_NAME_LEN -1);
	newDir->fileName[DIR_NAME_LEN -1] = '\0';
	// newDir->extentBlockStart = originalFile->extentBlockStart;
	newDir->fileSize = originalFile->fileSize;
	newDir->createdTime = originalFile->createdTime;
	newDir->modifiedTime = originalFile->modifiedTime;
	newDir->lastAccessedTime = originalFile->lastAccessedTime;
	// newDir->extentIndex = originalFile->extentIndex;
	newDir->isDirectory = originalFile->isDirectory;


    	markDirUnused(originalFile);


	// I think pathUpdate(), a helper function in mfsHelper.c, should be called somewhere in this function.


    	// Update (EXT and Dir) of Original and New
	EXTTABLE *extOriginal = loadExtent(ppiF->parent);
	int parentStartBlockOriginal = extOriginal[1].tableArray[0].start;
    	writeDir(ppiF->parent, parentStartBlockOriginal);
	pathUpdate((const char*) fileName);
	free(extOriginal);

        EXTTABLE *extNew = loadExtent(ppiD->parent);
	int parentStartBlockNew = extNew[1].tableArray[0].start;
    	writeDir(ppiD->parent, parentStartBlockNew);
	pathUpdate((const char*) destinationDir);
	free(extNew);
	
    	free(ppiF);
    	free(ppiD);

        return 0;
}
