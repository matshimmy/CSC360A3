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
void printFiles(aboutDisk *);
char *decodeDate(int );
char *decodeTime(int );


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

	aboutDisk *ad1;
	ad1 = (aboutDisk *)malloc(sizeof(aboutDisk));
	ad1->bytesPerSec = lEndian(mp,11,2);
	ad1->totalSec = lEndian(mp,19,2);
	ad1->movedSec = 0;
	ad1->curMp = mp;
	ad1->numOfFatCP = lEndian(ad1->curMp,16,1);
	ad1->secPerFAT = lEndian(ad1->curMp,22,2);
	/* find a file in the attribute system*/

	printFiles(ad1);

	munmap(mp,buf.st_size);
	close(fd);
}

void printFiles(aboutDisk *ad1){
	movToDir(ad1);
	int look = lEndian(ad1->curMp, 11, 1);
	char *extn = getStr(ad1->curMp,8,3); /*gets the extension*/
	int count = 0; /*count for total iterations*/
	int fileSz;
	int fileTime;
	int fileDate;
	char *date;
	char *time;
	char *fileName;
	while(count != 16){
		if ((look != 0xF) && (strcmp(extn,"   ") != 0) && (extn[0] != '\0')){ /*if it is not a long filename and it has a extension (that is not 000000) then it is a file*/
			fileName = getStr(ad1->curMp, 0, 8);
			fileSz = lEndian(ad1->curMp, 28, 4);
			fileTime = lEndian(ad1->curMp, 22, 2);
			fileDate = lEndian(ad1->curMp, 24, 2);
			date  = decodeDate(fileDate);
			time = decodeTime(fileTime);
			if(look == 0x10){ /*directory*/
				printf("D %10d bytes %20s \t%s \t%s\n", fileSz, fileName, time, date);
			}else{
				printf("F %10d bytes %20s \t%s \t%s\n", fileSz, fileName, time, date);
			}
		}
		ad1->curMp += 32; /*moves over to the next entry*/
		extn = getStr(ad1->curMp,8,3); /*gets the extension*/
		look = lEndian(ad1->curMp, 11, 1);
		count++;
	}
	ad1->curMp -= (count*32); /*resets the curMp*/
}

char *decodeDate(int hex){
	int year = hex >> 9;
	year += 1980;
	int month = (hex & 0x01E0)>>5;
	int date = (hex &0x1F);
	
	char *buf = (char*)malloc(sizeof(10));
	sprintf(buf ,"%d/%d/%d", year, month, date);
	return buf;
}

char *decodeTime(int hex){
	int hours = hex >> 11;
	int minutes = (hex & 0x07E0)>>5;
	int seconds = (hex &0x1F);
	
	char *buf = (char*)malloc(sizeof(10));
	sprintf(buf ,"%d:%d:%d", hours, minutes, seconds*2);
	return buf;
}
