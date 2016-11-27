#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include "diskOps.h"
#include "aboutDiskStruct.h"


void startPointer(char **disk);
void *getVol(aboutDisk*);/*gets the volume label*/
int getOcuSec(aboutDisk*); /*get number of occupied sectors*/
int countFiles(aboutDisk*);


int main(int argc,char **argv){

	if(argc <= 1){ /*no arguments*/
		printf("No Arguments\n");
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

	char *osname;
	osname = getStr(mp,3,8);
	printf("OS Name: %s\n",osname);

	aboutDisk *ad1;
	ad1 = (aboutDisk *)malloc(sizeof(aboutDisk));
	ad1->bytesPerSec = lEndian(mp,11,2);
	ad1->totalSec = lEndian(mp,19,2);
	ad1->movedSec = 0;
	ad1->curMp = mp;
	ad1->numOfFatCP = lEndian(ad1->curMp,16,1);
	ad1->secPerFAT = lEndian(ad1->curMp,22,2);
	/* find a file in the attribute system*/
	char *volLab = getVol(ad1);

	printf("Label of the disk: %s\n",volLab);



	int diskSz = ad1->totalSec*ad1->bytesPerSec;
	printf("Total size of the disk: %d bytes\n",diskSz);


	int diskFree = getOcuSec(ad1);
	printf("Free size of the disk: %d bytes\n\n==============\n", diskSz - (diskFree * ad1->bytesPerSec));

	int rootF = countFiles(ad1);
	printf("The number of files in the root directory (not including subdirectories): %d\n\n=============\n",rootF);
	

	printf("Number of FAT copies: %d\n", ad1->numOfFatCP);


	printf("Sectors per FAT: %d\n",ad1->secPerFAT);
	munmap(mp,buf.st_size);
	close(fd);
}

void *getVol(aboutDisk *ad1){
	movToBoot(ad1);
	char *volLab = getStr(ad1->curMp,43,11);
	if(0 == strcmp(volLab,"           ")){ /*if there is nothing then move on*/
		movToDir(ad1);
		int count = 0;
		int look = lEndian(ad1->curMp, 11, 1);
		while(0x08 != look && count != 16){
			ad1->curMp += 32; /*moves over to the next entry*/
			look = lEndian(ad1->curMp, 11, 1);
			count++;
		}
		if (count == 16){
			ad1->curMp -= (count*32); /*resets the curMp*/
			return "error: no volume found";
		}
		volLab = getStr(ad1->curMp,0,8);
		ad1->curMp -= (count*32); /*resets the curMp*/
	}
	return volLab;
}

int countFiles(aboutDisk *ad1){
	int countF = 0; /*count for files*/
	movToDir(ad1);
	int look = lEndian(ad1->curMp, 11, 1);
	char *extn = getStr(ad1->curMp,8,3); /*gets the extension*/
	int count = 0; /*count for total iterations*/
	while(count != 16*14){
		ad1->curMp += 32; /*moves over to the next entry*/
		if ((look != 0xF) && (strcmp(extn,"   ") != 0) && (extn[0] != '\0')){ /*if it is not a long filename and it has a extension (that is not 000000) then it is a file*/
			countF++;
		}
		extn = getStr(ad1->curMp,8,3); /*gets the extension*/
		look = lEndian(ad1->curMp, 11, 1);
		count++;
	}
	ad1->curMp -= (count*32); /*resets the curMp*/
	return countF;
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
