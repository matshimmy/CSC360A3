all: diskinfo disklist

diskinfo: diskinfo.o diskOps.o
	gcc -Wall diskinfo.o diskOps.o -o diskinfo

disklist: disklist.o diskOps.o
	gcc -Wall disklist.o diskOps.o -o disklist

disklist.o: disklist.c
	gcc -Wall -c disklist.c

diskinfo.o: diskinfo.c
	gcc -Wall -c diskinfo.c 

diskOps.o: diskOps.c
	gcc -Wall -c diskOps.c

clean:
	rm -f *.o diskinfo disklist
