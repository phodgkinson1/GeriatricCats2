#ifndef MFSHELPER_H
#define MFSHELPER_H
#include "freespace.h"
#include "directoryEntry.h"

// Global variables
extern DE * rootDir;
extern DE * cwd;
extern int cwdGlobal;
extern char *absolutePath;

typedef struct parsePathInfo
{
    DE *parent;
    char *lastElement;
    int indexOfLastElement;

} parsePathInfo;

int writeExtent(DE * dir, EXTTABLE * ext);
int writeDir(DE * dir, int location);
int parsePath(char * path, parsePathInfo * ppi);
int FindEntryInDir(DE * dir, char * fileName);
int isDirectory(DE * entry);
int isDirEmpty(DE *dir);
void markDirUnused(DE *dir);


EXTTABLE * loadExtent(DE * dir);
DE * loadDir(DE * dir, int index);


#endif // MFSHELPER_H
