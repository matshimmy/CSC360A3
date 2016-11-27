#ifndef DiskStructDef
#define DiskStructDef
typedef struct {
	int bytesPerSec;
	int totalSec;
	int movedSec;
	int numOfFatCP;
	int secPerFAT;
	char * curMp;
}aboutDisk;
#endif
