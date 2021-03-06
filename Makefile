all: diskinfo disklist diskget diskput

diskinfo: diskinfo.o diskOps.o
	gcc -Wall diskinfo.o diskOps.o -o diskinfo

disklist: disklist.o diskOps.o
	gcc -Wall disklist.o diskOps.o -o disklist

diskget: diskget.o diskOps.o
	gcc -Wall diskget.o diskOps.o -o diskget

diskput: diskput.o diskOps.o
	gcc -Wall diskput.o diskOps.o -o diskput

diskput.o: diskput.c
	gcc -Wall -c diskput.c

diskget.o: diskget.c
	gcc -Wall -c diskget.c

disklist.o: disklist.c
	gcc -Wall -c disklist.c

diskinfo.o: diskinfo.c
	gcc -Wall -c diskinfo.c 

diskOps.o: diskOps.c
	gcc -Wall -c diskOps.c

clean:
	rm -f *.o diskinfo disklist diskget diskput
