all: diskinfo

diskinfo: diskinfo.c
	gcc -Wall diskinfo.c -o diskinfo

clean:
	rm -f *.o diskinfo
