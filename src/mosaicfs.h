#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

struct mosaic {
    char dir[PATH_MAX];
    int format;
    off_t size;
    off_t tile_size;
    long int ntiles;
    int *fd;
    int nfiles;
    int nfiles_max;
};

static pthread_mutex_t the_lock;

int mosaicfs_getattr(const char *path, struct stat *stbuf);
int mosaicfs_truncate(const char *path, off_t nsize);
int mosaicfs_open(const char *path, struct fuse_file_info *fi);
int mosaicfs_read(const char *path, char *buf, size_t size, off_t offset,
			 struct fuse_file_info *fi);
int mosaicfs_write(const char *path, const char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi);
int mosaicfs_release(const char * path, struct fuse_file_info * fi);

