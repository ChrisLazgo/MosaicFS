#include "mosaicfs.h"
#include "global.h"

extern struct mosaic mosaic;

static int get_tile_name(long int ntile, char * name) {

    const char b32[] = "0123456789abcdefghijklmnopqrstuv";
    int i, n;

    strcpy(name, "../....");
    for (i = 0; i < 4; i++) {
        n = ntile & 0b011111;
        ntile = ntile >> 5;
        name[7-i-1] = b32[n];
    }
    memcpy(&name[0],&name[3],2);

    return 0;
}

static int close_tile_files() {

    long int i;
    for (i = 0; i < mosaic.ntiles; i++) {
        if (mosaic.fd[i] != 0) {
            close(abs(mosaic.fd[i]));
            mosaic.nfiles--;
            mosaic.fd[i] = 0;
        }
    }
    return 0;
}

static int open_tile_read(const off_t tile_num) {

    int fd;
    char fpath[PATH_MAX];
    char tile_name[32];

    // Close all files if we have reached the limit
    if ( mosaic.nfiles == mosaic.nfiles_max)
        close_tile_files();

    // Tile full path name
    get_tile_name(tile_num, tile_name);
    snprintf(fpath, sizeof(fpath), "%s/%s", mosaic.dir, tile_name);

    // Open new file
    fd = open(fpath,O_RDONLY);
    if ( fd < 0 ) {
        // File is stealth (does not exist)
        fd = 0;
    }
    else {
        // File exists and has been opened
        mosaic.nfiles++;
    }
        
    return fd;
}


static int open_tile_write(const off_t tile_num) {

    int dir_is_new;
    int file_is_new;
    int fd;
    int rv;
    char tile_name[32];
    char fpath[PATH_MAX], *dir_name ;
    struct stat buf;

    // Close all files if we have reached the limit
    if ( mosaic.nfiles == mosaic.nfiles_max)
        close_tile_files();

    // Tile full path name
    get_tile_name(tile_num, tile_name);
    snprintf(fpath, sizeof(fpath), "%s/%s", mosaic.dir, tile_name);

    // Create sub-directory if necessary
    dir_name = strdup(fpath);
    dirname(dir_name);
    dir_is_new = (stat(dir_name, &buf) !=0 );
    if (dir_is_new) {
      mkdir(dir_name,S_IRWXU);
    }

    // Open file
    file_is_new = (stat(fpath, &buf) !=0 );
    fd = open(fpath,O_RDWR|O_CREAT,0600);
    if ( fd < 0 ) {
	fprintf(stderr, "ERROR: could not create tile file %s\n",fpath);
	fprintf(stderr, "ERROR: mosaic_nfiles = %d\n",mosaic.nfiles);
	fprintf(stderr, "ERROR: errno = %d\n",errno);
	exit(-1);
    }
    mosaic.nfiles++;

    // If this is a new file, make it of desired size
    if (file_is_new) {
        rv = pwrite(fd, "\0", 1, mosaic.tile_size-1);
        if (rv < 0) {
	    fprintf(stderr, "ERROR writing to tile file %s\n",fpath);
	    exit(-1);
        }
    }
        
    return fd;
}

static int get_tile_read(const off_t tile_num) {

    // File is not open: open for reading
    if (mosaic.fd[tile_num] == 0) {
        mosaic.fd[tile_num] = -open_tile_read(tile_num);
    }

    return abs(mosaic.fd[tile_num]);
}

static int get_tile_write(const off_t tile_num) {

    // File is open for reading: close it first
    if (mosaic.fd[tile_num] < 0) {
        close(-mosaic.fd[tile_num]);
        mosaic.nfiles--;
        mosaic.fd[tile_num] = 0;
    }

    // File is not open: open for writing
    if (mosaic.fd[tile_num] == 0) {
        mosaic.fd[tile_num] = open_tile_write(tile_num);
    }

    return mosaic.fd[tile_num];
}

static void lock()
{
	pthread_mutex_lock(&the_lock);
}

static void unlock()
{
	pthread_mutex_unlock(&the_lock);
}

int mosaicfs_getattr(const char *path, struct stat *stbuf)
{
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    stbuf->st_mode = S_IFREG | 0600;
    stbuf->st_nlink = 1;
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    stbuf->st_size = mosaic.size;
    stbuf->st_blocks = 0;
    stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);

    return 0;
}

int mosaicfs_open(const char *path, struct fuse_file_info *fi)
{
    //fprintf(stderr,"--> mosaicfs_open: %s\n",path);
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    return 0;
}

int mosaicfs_release(const char * path, struct fuse_file_info * fi)
{
    //fprintf(stderr,"--> mosaicfs_release: %s\n",path);
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    return close_tile_files();
}

int mosaicfs_read(const char *path, char *buf, size_t size, off_t offset,
			 struct fuse_file_info *fi)
{
    int fd;
    off_t tile_num, tile_offset;
    size_t bufpos, r;

    //fprintf(stderr,"--> mosaicfs_read: begin\n");
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    size = min( size, max(mosaic.size-offset,(off_t)0) );
    for (bufpos = 0; bufpos < size; ) {
        tile_num    = (offset+(off_t)bufpos) / mosaic.tile_size;
        tile_offset = (offset+(off_t)bufpos) % mosaic.tile_size;
        lock();
        fd = get_tile_read(tile_num);
        if ( fd  > 0 ) {
            r = pread(fd,buf+bufpos,size-bufpos,tile_offset);
            bufpos += r;
        } else {
            r = min(size-bufpos,mosaic.tile_size-tile_offset);
            memset(buf+bufpos,0,r);
            bufpos += r;
        }
        unlock();
    }
    return bufpos;
}

int mosaicfs_write(const char *path, const char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{

    int fd;
    off_t tile_num, tile_offset;
    size_t bufpos, r, rv;

    //fprintf(stderr,"--> mosaicfs_write: %s\n",path);
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    size = min( size, max(mosaic.size-offset, (off_t)0) );
    for (bufpos = 0; bufpos < size; ) {
        tile_num    = (offset+(off_t)bufpos) / mosaic.tile_size;
        tile_offset = (offset+(off_t)bufpos) % mosaic.tile_size;
        lock();
        fd = get_tile_write(tile_num);
        r = min(size-bufpos, mosaic.tile_size-tile_offset);
        rv = pwrite(fd,buf+bufpos,r,tile_offset);
        unlock();
        if (rv < 0) {
            return -errno;
        }

        bufpos += r;
    }

    return bufpos;
}

int mosaicfs_truncate(const char *path, off_t nsize)
{
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    return 0;
}
