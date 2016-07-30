CC	 = gcc
CFLAGS	 = -Wall $(PKGFLAGS)
LDFLAGS	 = $(PKGFLAGS)
PKGFLAGS = `pkg-config fuse --cflags --libs`

objects = main.o parse.o mosaicfs.o

all: mosaicfs

main.o: main.c mosaicfs.h parse.h global.h

parse.o: parse.c parse.h global.h

mosaicfs.o: mosaicfs.c mosaicfs.h global.h

mosaicfs: $(objects)
	$(CC) $(objects) $(LDFLAGS) -o mosaicfs

clean:
	rm -f $(objects) mosaicfs
