#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include "diskOps.h"
#include "aboutDiskStruct.h"


void startPointer(char **disk);
int findNextDir(aboutDisk*);/*returns offset from root directory*/
int writeToDir(aboutDisk*, FILE *, int, struct stat, char *);
int getOcuSec(aboutDisk *);
int nextEmptyClust(aboutDisk* );
void writeToFAT(aboutDisk* , int , FILE *, int );
void writeToMemory(aboutDisk* , int , FILE *, FILE *, struct stat );

int main(int argc,char **argv){

	if(argc <= 2){ /*no arguments*/
		printf("Not enough arguments\n");
	}else{
		char *tmpp = argv[1];
		int len = strlen(argv[1]);
		if(strcasecmp(".ima",tmpp+len - 4) == 0){ /*check if it is a disk file*/
			startPointer(argv);
		}else{
			printf("Input does not have the .ima extension\n");
		}
	}
	return 0;
	
}

void startPointer(char **disk){
	int fd;
	fd = open(disk[1],O_RDONLY);
	struct stat buf;
	fstat(fd, &buf); /*to get file size*/
	char *mp;
	mp = mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, fd, 0);

	aboutDisk *ad1;
	ad1 = (aboutDisk *)malloc(sizeof(aboutDisk));
	ad1->bytesPerSec = lEndian(mp,11,2);
	ad1->totalSec = lEndian(mp,19,2);
	ad1->movedSec = 0;
	ad1->curMp = mp;
	ad1->numOfFatCP = lEndian(ad1->curMp,16,1);
	ad1->secPerFAT = lEndian(ad1->curMp,22,2);
	int diskSz = ad1->totalSec*ad1->bytesPerSec;
	int diskFree = getOcuSec(ad1);
	diskFree = diskSz - (diskFree * ad1->bytesPerSec);

	int statFile = open(disk[2], O_RDONLY);
	if(statFile >= 0){
		struct stat af; /*file info*/
		FILE *addFile = fopen(disk[2], "r");
		fstat(statFile, &af);
		if(diskFree <= af.st_size){
			printf("Not enough free space in disk\n");
		}else{
			int nextDir = findNextDir(ad1);
			FILE *diskImag = fopen(disk[1], "r+b"); /*opens disk image in binary*/
			int startClust = writeToDir(ad1, diskImag, nextDir, af, disk[2]);

			writeToMemory(ad1, startClust, diskImag, addFile, af);

			fclose(diskImag);
		}
		fclose(addFile);
	}else{
		printf("file not found\n");
	}



	munmap(mp,buf.st_size);
	close(statFile);
	close(fd);
}

void writeToMemory(aboutDisk* ad1, int startClust, FILE *diskImag, FILE *addFile, struct stat af){
	int cpLeft = af.st_size; /*how much is left to copy*/
	int countSec = 0; /*file sector count*/
	int logSec; /*seek logical sector*/
	char *cpSec = (char *)malloc(sizeof(char)*ad1->bytesPerSec); /*copied sector from file */
	int oldClust = startClust;
	writeToFAT(ad1, oldClust, diskImag, 0xFFF); /*startClust is now the next cluster*/
	int nwClust = nextEmptyClust(ad1);
	while (cpLeft > 0){
		if(cpLeft < ad1->bytesPerSec){
			fseek(addFile, countSec*ad1->bytesPerSec, SEEK_SET); /*reads the next sector*/
			fread(cpSec, 1, cpLeft, addFile); /*copies whats left*/

			cpLeft = 0;

			logSec = (31 + oldClust) * ad1->bytesPerSec; /*copies the next sector to the disk*/
			fseek(diskImag, logSec, SEEK_SET);
			fwrite(cpSec, 1, ad1->bytesPerSec, diskImag);

		}else{
			cpLeft -= ad1->bytesPerSec;

			fseek(addFile, countSec*ad1->bytesPerSec, SEEK_SET); /*reads the next sector*/
			fread(cpSec, 1, ad1->bytesPerSec, addFile);

			logSec = (31 + oldClust) * ad1->bytesPerSec; /*copies the next sector to the disk*/
			fseek(diskImag, logSec, SEEK_SET);
			fwrite(cpSec, 1, ad1->bytesPerSec, diskImag);

			writeToFAT(ad1, oldClust, diskImag, nwClust); /*startClust is now the next cluster*/
			oldClust = nwClust;
			writeToFAT(ad1, oldClust, diskImag, 0xFFF);
			nwClust = nextEmptyClust(ad1);
			countSec++; /*next sector of addFile*/

		}
	}
}

void writeToFAT(aboutDisk* ad1, int index, FILE *diskImag, int wValue){
	int wIndex; /*write to index*/
	if (index%2 == 0){ /*even*/
		int low4loc = 1+(3*index)/2;
		int high8loc = (3*index)/2;
		char high8val[1];
		char low4val[1];
		high8val[0] = 0xFF & wValue;
		low4val[0] = lEndian(ad1->curMp, low4loc,1) & 0xF0;
		low4val[0] |= (wValue >> 8);
		char store[2];
		store[0] = high8val[0];
		store[1] = low4val[0];
		wIndex = high8loc + ad1->bytesPerSec; 
		fseek(diskImag, wIndex, SEEK_SET); 
		fwrite(store, 1, 2, diskImag);
	}else{ /*odd*/
		int high4loc = (3*index)/2;
		char high4val[1];
		char low8val[1];
		high4val[0] = lEndian(ad1->curMp, high4loc,1) & 0x0F;
		high4val[0] |= (wValue<<4);
		low8val[0] = wValue>>4;
		char store[2];
		store[0] = high4val[0];
		store[1] = low8val[0];
		wIndex = high4loc + ad1->bytesPerSec; 
		fseek(diskImag, wIndex, SEEK_SET); 
		fwrite(store, 1, 2, diskImag);
	}
}

int nextEmptyClust(aboutDisk* ad1){
	int x = 1;
	int FATVal = getFATVal(x, ad1);
	while (FATVal != 0){
		x++;
		FATVal = getFATVal(x, ad1);
	}
	return x;
}

int writeToDir(aboutDisk *ad1, FILE *diskImag, int dir, struct stat af, char * fileName){
	int logDir = (ad1->bytesPerSec * 19) + dir; /*physical directory*/
	fseek(diskImag, logDir, SEEK_SET); /*go to directory*/
	char dirData[32];
	int x = 0;
	while(fileName[x] != '.'){ /*copies the name over*/
		dirData[x] = fileName[x];
		x++;
	}
	fileName = fileName + x + 1; /*to get to the extension*/
	if( x > 8){
		printf("error file is longer than 8 characters\n");
		exit(0);
	}
	while(x < 8){ /*fills the rest with spaces*/
		dirData[x] = 0x20;
		x++;
	}
	int y = 0; /*new counter for extension*/
	while(fileName[y] != '\0'){ /*copies the extention over*/
		dirData[x] = fileName[y];
		y++;
		x++;
	}
	if( x > 11){
		printf("error extension is longer than 3 characters\n");
		exit(0);
	}
	while(x < 11){ /*fills the rest with spaces*/
		dirData[x] = 0x20;
		x++;
	}
	while(x < 14){ /*attributes and reserved*/
		dirData[x] = 0x0;
		x++;
	}

	tzset();
	time_t rawtime;
	time( &rawtime);
	struct tm *createTim;

	createTim = localtime( &rawtime );
	int year = createTim->tm_year - 80;
	int month = createTim->tm_mon + 1;
	int day = createTim->tm_mday;

	dirData[16] = 0xFF & (day | (month << 5));
	dirData[17] = 0xFF & ((month >> 3) | (year << 1));


	int hour = createTim->tm_hour;
	int min = createTim->tm_min;
	int sec = (int)(createTim->tm_sec/2);

	dirData[14] = 0xFF & (sec | (min<<5));
	dirData[15] = 0xFF & ((min >> 3) | (hour << 3));

	x = 18;
	while(x < 26){
		dirData[x] = 0x0;
		x++;
	}

	int emptClust = nextEmptyClust(ad1);

	dirData[27] = 0xFF  & (emptClust>>8);
	dirData[26] = 0xFF & emptClust;

	int stSize = (int)af.st_size;
	dirData[31] = 0xFF & (stSize >> 24);
	dirData[30] = 0xFF & (stSize >> 16);
	dirData[29] = 0xFF & (stSize >> 8);
	dirData[28] = 0xFF & (stSize);

	fwrite(dirData, 1, 32, diskImag);
	return emptClust;

}




int findNextDir(aboutDisk* ad1){
	movToDir(ad1);
	int count = 0; /*count for total iterations*/
	int x;
	char *fileName;
	while(count != 16*14){
		fileName = getStr(ad1->curMp, 0, 8);
		if (fileName[0] == 0x00){
			for(x = 1; x < 8; x++){
				if(fileName[x] != 0x0){
					x = 9;
				}
			}
			if( x == 8){ /*empty directory found*/
				/*printf("%d\n",count*32);*/
				break;
			}
		}
		ad1->curMp += 32; /*moves over to the next entry*/
		count++;
	}
	
	ad1->curMp -= (count*32); /*resets the curMp*/
	return (count*32); /*returns the offset of the next open dir*/
}


int getOcuSec(aboutDisk *ad1){
	int x;
	movToFAT(ad1);
	int numEntInFAT = (ad1->bytesPerSec * ad1->secPerFAT) /3; /*bytes per sector * 9 sectors in FAT / 3 bytes in entry*/
	int countEnt = 0;
	for(x = 0; x < (numEntInFAT); x++){
		if (getFATVal(x,ad1) != 0){
			countEnt++;
		}
	}
	return countEnt;
	
}
