# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

fdbuf.o: fdbuf.c fdbuf.h
	$(CC) $(CFLAGS) -c fdbuf.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

cache_wr.o: cache_wr.c cache_wr.h
	$(CC) $(CFLAGS) -c cache_wr.c

proxy: proxy.o csapp.o fdbuf.o cache.o cache_wr.o
	$(CC) $(CFLAGS) proxy.o csapp.o fdbuf.o cache.o cache_wr.o -o proxy $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz

