#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

struct aboutDisk{
	int bytesPerSec;
	int movedSec;
	char * curMp;
};

struct aboutDisk ad1;

void startPointer(char **disk);
int lEndian(char *, int,int ); /*converts  from little endian byte to int*/
void *getStr(char *, int, int); /*get string from memory*/
void movToBoot(); /*moves to boot sector*/
void movToFAT();/*moves to FAT sector*/
void movToDir();/*moves to root directory*/
void movToLog();/*moves to logical sector*/


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

	ad1.bytesPerSec = lEndian(mp,11,2);
	ad1.movedSec = 0;
	ad1.curMp = mp;
	movToDir();
	char *fName;
	fName = getStr(ad1.curMp,0,8);

	printf("File Name: %s\n",fName);
	munmap(mp,buf.st_size);
	close(fd);
}

void movToBoot(){
	ad1.curMp = ad1.curMp - ad1.movedSec;
	ad1.movedSec = 0;
}
void movToFAT(){
	ad1.curMp = ad1.curMp + ad1.bytesPerSec; /*just moved over one sec*/
	ad1.movedSec = ad1.bytesPerSec;
	
}
void movToDir(){
	movToBoot();
	ad1.curMp = ad1.curMp + (ad1.bytesPerSec*19);
	ad1.movedSec = ad1.bytesPerSec*19;

}
void movToLog(){
	
}

void *getStr(char *mp,int start,int len){
	char *source = (char *)malloc(len+1);
	memcpy(source, mp+start,len);
	source[len] = '\0'; /*since it is an array it starts from 0*/
	return source;
}

int lEndian(char *mp, int start,int len){ /*start byte and len*/
	char nwptr[1] = {'a'};/*a single byte*/
	int final = 0;
	for (;len != 0; len--){
		nwptr[0] = (mp + start + len - 1)[0]; /*start at the bottom bit*/
		final = (final << 8) + nwptr[0]; /*adds the word and shifts the int over 8*/
	}
	return final;
}
