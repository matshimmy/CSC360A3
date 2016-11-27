#include "aboutDiskStruct.h"

void movToBoot(aboutDisk*); /*moves to boot sector*/
void movToFAT(aboutDisk*);/*moves to FAT sector*/
void movToDir(aboutDisk*);/*moves to root directory*/
void *getStr(char *, int, int); /*get string from memory*/
int lEndian(char *, int,int ); /*converts  from little endian byte to int*/
int getFATVal(int, aboutDisk*);
