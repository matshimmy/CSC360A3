all: diskinfo disklist diskget

diskinfo: diskinfo.o diskOps.o
	gcc -Wall diskinfo.o diskOps.o -o diskinfo

disklist: disklist.o diskOps.o
	gcc -Wall disklist.o diskOps.o -o disklist

diskget: diskget.o diskOps.o
	gcc -Wall diskget.o diskOps.c -o diskget

diskget.o: diskget.c
	gcc -Wall -c diskget.c

disklist.o: disklist.c
	gcc -Wall -c disklist.c

diskinfo.o: diskinfo.c
	gcc -Wall -c diskinfo.c 

diskOps.o: diskOps.c
	gcc -Wall -c diskOps.c

clean:
	rm -f *.o diskinfo disklist diskget
