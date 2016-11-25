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
	int totalSec;
	int movedSec;
	int numOfFatCP;
	int secPerFAT;
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
void *getVol();/*gets the volume label*/
int getFATVal(int);
int getOcuSec(); /*get number of occupied sectors*/
int countFiles();


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
	ad1.totalSec = lEndian(mp,19,2);
	ad1.movedSec = 0;
	ad1.curMp = mp;
	ad1.numOfFatCP = lEndian(ad1.curMp,16,1);
	ad1.secPerFAT = lEndian(ad1.curMp,22,2);
	/* find a file in the attribute system*/
	char *volLab = getVol();

	printf("Label of the disk: %s\n",volLab);



	int diskSz = ad1.totalSec*ad1.bytesPerSec;
	printf("Total size of the disk: %d bytes\n",diskSz);


	int diskFree = getOcuSec();
	printf("Free size of the disk: %d bytes\n\n==============\n", diskSz - (diskFree * ad1.bytesPerSec));

	int rootF = countFiles();
	printf("The number of files in the root directory (not including subdirectories): %d\n\n=============\n",rootF);
	

	printf("Number of FAT copies: %d\n", ad1.numOfFatCP);


	printf("Sectors per FAT: %d\n",ad1.secPerFAT);
	munmap(mp,buf.st_size);
	close(fd);
}

int countFiles(){
	int countF = 0; /*count for files*/
	movToDir();
	int look = lEndian(ad1.curMp, 11, 1);
	char *extn = getStr(ad1.curMp,8,3); /*gets the extension*/
	int count = 0; /*count for total iterations*/
	while(count != 16){
		ad1.curMp += 32; /*moves over to the next entry*/
		if ((look != 0xF) && (strcmp(extn,"   ") != 0) && (extn[0] != '\0')){ /*if it is not a long filename and it has a extension (that is not 000000) then it is a file*/
			countF++;
		}
		extn = getStr(ad1.curMp,8,3); /*gets the extension*/
		look = lEndian(ad1.curMp, 11, 1);
		count++;
	}
	ad1.curMp -= (count*32); /*resets the curMp*/
	return countF;
}

int getOcuSec(){
	int x;
	movToFAT();
	int numEntInFAT = (ad1.bytesPerSec * ad1.secPerFAT) /3; /*bytes per sector * 9 sectors in FAT / 3 bytes in entry*/
	int countEnt = 0;
	for(x = 0; x < (numEntInFAT); x++){
		if (getFATVal(x) != 0){
			countEnt++;
		}
	}
	return countEnt;
	
}

int getFATVal(int index){
	movToFAT();
	int FATVal;
	if (index%2 == 0){ /*even*/
		int low4 = 1+(3*index)/2;
		int high8 = (3*index)/2;
		low4 = lEndian(ad1.curMp, low4,1);
		high8 = lEndian(ad1.curMp, high8, 1);
		FATVal = (low4<<8) | high8; /*move over 2 bytes*/
		FATVal &= 0x0FFF; /*take off the top value*/
		/*printf("%x\n",FATVal);*/
	}else{ /*odd*/
		int high4 = (3*index)/2;
		int low8 = 1 + (3*index)/2;
		high4 = lEndian(ad1.curMp, high4,1);
		low8 = lEndian(ad1.curMp, low8,1);
		high4 &= 0xF0; /*take off the bottom byte*/
		FATVal = (low8<<4) | (high4>>4); /*move over 1 byte*/
		/*printf("%x\n",high4);*/
	}

	return FATVal;
}

void *getVol(){
	movToBoot();
	char *volLab = getStr(ad1.curMp,43,11);
	if(0 == strcmp(volLab,"           ")){ /*if there is nothing then move on*/
		movToDir();
		int count = 0;
		int look = lEndian(ad1.curMp, 11, 1);
		while(0x08 != look && count != 16){
			ad1.curMp += 32; /*moves over to the next entry*/
			look = lEndian(ad1.curMp, 11, 1);
			count++;
		}
		if (count == 16){
			ad1.curMp -= (count*32); /*resets the curMp*/
			return "error: no volume found";
		}
		volLab = getStr(ad1.curMp,0,8);
		ad1.curMp -= (count*32); /*resets the curMp*/
	}
	return volLab;
}

void movToBoot(){
	ad1.curMp = ad1.curMp - ad1.movedSec;
	ad1.movedSec = 0;
}
void movToFAT(){
	movToBoot();
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
