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
int getToFile(aboutDisk*, char *); /*returns offset from root directory*/
void makeFile(aboutDisk*, char*, int, int*, int);

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

	int fileOffset;
	fileOffset = getToFile(ad1, disk[2]);
	if (fileOffset < 0){
		printf("File not found.\n");
	}else{
		int firstClust = lEndian(ad1->curMp, fileOffset + 26, 2);
		int fileSz = lEndian(ad1->curMp, fileOffset + 28, 4);
		int totalClust = 0;
		int *clusts = countClust(ad1, firstClust, &totalClust);
		makeFile(ad1, disk[2], fileSz, clusts, totalClust);
	}

	munmap(mp,buf.st_size);
	close(fd);
}

void makeFile(aboutDisk *ad1, char *fileName, int fileSize, int *clusts, int totalClust){
	movToBoot(ad1);
	FILE *newFile = fopen(fileName, "w");
	int x;
	char *portion = (char *)malloc(sizeof(char) * ad1->bytesPerSec);
	int physClust; /*will hold the physical cluster*/
	for(x = 0; x < totalClust; x++){
		physClust = clusts[x] + 31;
		portion = memcpy(portion, ad1->curMp + (physClust * ad1->bytesPerSec), ad1->bytesPerSec);
		if(x == totalClust-1){
			fwrite(portion, 1, fileSize, newFile);
		}else{
			fwrite(portion, 1, ad1->bytesPerSec, newFile);
		}
	}
	fclose(newFile);
	
}

int getToFile(aboutDisk *ad1, char *arg){
	movToDir(ad1);
	int look = lEndian(ad1->curMp, 11, 1);
	char *extn = getStr(ad1->curMp,8,3); /*gets the extension*/
	int count = 0; /*count for total iterations*/
	char *fileName;
	int x;
	int y;
	char *buf = (char *)malloc(1 + strlen(arg));
	while(count != 16*14){
		if ((look != 0xF) && (strcmp(extn,"   ") != 0) && (extn[0] != '\0')){ /*if it is not a long filename and it has a extension (that is not 000000) then it is a file*/
			fileName = getStr(ad1->curMp, 0, 8);
			x = 7; /*the start of the string*/
			while(fileName[x] == 0x20 || x == -1){ /*remove all spaces from file*/
				fileName[x] = '\0';
				x--;
			}
			y = 3;
			while(extn[y] == 0x20 || y == -1){ /*remove all spaces from extension*/
				extn[y] = '\0';
				y--;
			}

			sprintf(buf, "%s.%s", fileName, extn);
			buf[x+y+2] = '\0';
			if (strcmp(buf, arg) == 0){ /*file found*/
				ad1->curMp -= (count*32); /*resets the curMp*/
				return count*32; /*this is the offset*/
			}
		}
		ad1->curMp += 32; /*moves over to the next entry*/
		extn = getStr(ad1->curMp,8,3); /*gets the extension*/
		look = lEndian(ad1->curMp, 11, 1);
		count++;
	}
	ad1->curMp -= (count*32); /*resets the curMp*/
	return -1; /*file not found*/
}
