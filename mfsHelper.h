#ifndef MFSHELPER_H
#define MFSHELPER_H
#include "freespace.h"
#include "directoryEntry.h"

// Global variables
DE * rootDir;
DE * cwd;
int cwdGlobal;

typedef struct parsePathInfo
{
    DE *parent;
    char *lastElement;
    int indexOfLastElement;

} parsePathInfo;

int writeDir(DE * dir);
int parsePath(char * path, parsePathInfo * ppi);
int FindEntryInDir(DE * dir, char * fileName);
int isDirectory(DE * entry);
EXTTABLE * loadExtent(DE * dir);
DE * loadDir(DE * dir, int index);


#endif // MFSHELPER_H
