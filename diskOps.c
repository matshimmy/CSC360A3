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

int *countClust(aboutDisk* ad1, int firstClust, int *totalClusts){
	int x = 1;
	int FATVal = getFATVal(firstClust, ad1);
	while (FATVal < 0xFF0 && FATVal != 0){
		FATVal = getFATVal(FATVal, ad1);
		x++;
	}
	int *clusts = (int *)malloc(sizeof(int)*x);
	*totalClusts = x;
	clusts[0] = firstClust;
	x = 1;
	FATVal = getFATVal(firstClust, ad1);
	while (FATVal < 0xFF0 && FATVal != 0){

		clusts[x] = FATVal;
		x++;
		FATVal = getFATVal(FATVal, ad1);

	}
	
	return clusts;
}

int getFATVal(int index, aboutDisk *ad1){
	movToFAT(ad1);
	int FATVal;
	if (index%2 == 0){ /*even*/
		int low4 = 1+(3*index)/2;
		int high8 = (3*index)/2;
		low4 = lEndian(ad1->curMp, low4,1);
		high8 = lEndian(ad1->curMp, high8, 1);
		FATVal = (low4<<8) | high8; /*move over 2 bytes*/
		FATVal &= 0x0FFF; /*take off the top value*/
		/*printf("%x\n",FATVal);*/
	}else{ /*odd*/
		int high4 = (3*index)/2;
		int low8 = 1 + (3*index)/2;
		high4 = lEndian(ad1->curMp, high4,1);
		low8 = lEndian(ad1->curMp, low8,1);
		high4 &= 0xF0; /*take off the bottom byte*/
		FATVal = (low8<<4) | (high4>>4); /*move over 1 byte*/
		/*printf("%x\n",high4);*/
	}

	return FATVal;
}



void movToBoot(aboutDisk *ad1){
	ad1->curMp = ad1->curMp - ad1->movedSec;
	ad1->movedSec = 0;
}
void movToFAT(aboutDisk *ad1){
	movToBoot(ad1);
	ad1->curMp = ad1->curMp + ad1->bytesPerSec; /*just moved over one sec*/
	ad1->movedSec = ad1->bytesPerSec;
	
}
void movToDir(aboutDisk *ad1){
	movToBoot(ad1);
	ad1->curMp = ad1->curMp + (ad1->bytesPerSec*19);
	ad1->movedSec = ad1->bytesPerSec*19;

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
		final = (final << 8) + (0xFF & nwptr[0]); /*adds the word and shifts the int over 8*/
	}
	return final;
}
