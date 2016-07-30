/*
 *  MosaicFS is a filesystem in userspace (FUSE) allowing read and write
 *  access to MosaicFS archives. 
 *
 *  When mounted, a MosaicFS archive is presented as a single large file, 
 *  the content of which is stored in a number of smaller fixed size 
 *  individual tile files. Tile files are created dynamically as needed, 
 *  making MosaicFS archives sparse images.
 *
 *  Copyright (c) 2016 Chris Golaz <aaaa@aaaa>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "mosaicfs.h"
#include "global.h"
#include "parse.h"

// Mapping functions for FUSE
static struct fuse_operations mosaicfs_oper = {
    .getattr    = mosaicfs_getattr,
    .truncate   = mosaicfs_truncate,
    .open       = mosaicfs_open,
    .read       = mosaicfs_read,
    .write      = mosaicfs_write,
    .release    = mosaicfs_release,
};

// Global variables
struct mosaic mosaic;
struct args args;

// MosaicFS mount
int mount_cmd(int argc, char **argv)
{
    FILE* fd;

    // Initialize lock
    pthread_mutex_init(&the_lock, NULL);

    // Construct arguments to pass to FUSE
    int fuse_argc;
    char ** fuse_argv = NULL;
    fuse_argv = realloc (fuse_argv, 2*sizeof(char*));
    fuse_argv[0] = argv[0];
    fuse_argv[1] = argv[optind+2];
    fuse_argc = 2;
    if (args.fuse != NULL) {
        // Process optional FUSE arguments if present
        int n = fuse_argc;
        char *str;
        str = (char *)malloc(sizeof(char)*(strlen(args.fuse)+1));
        strcpy(str,args.fuse);
        char *p = strtok (str, " ");
        while (p)
        {
            fuse_argv = realloc (fuse_argv, sizeof (char*) * ++n);
            fuse_argv[n-1] = p;
            p = strtok (NULL, " ");
        }
        fuse_argc = n;
    }

    /*
    int i;
    for (i = 0; i < fuse_argc; ++i)
      printf ("fuse_argv[%d] = %s\n", i, fuse_argv[i]);
    */
 
    // Path of mosaicfs archive
    if (argv[optind+1][0] == '/') {
        // Mount point is absoloute path
        strncpy(mosaic.dir, argv[optind+1], sizeof(mosaic.dir));
    } else {
        // Mount point is relative path: convert to absolute
        char cwd[PATH_MAX];
        getcwd(cwd, sizeof(cwd));
        snprintf(mosaic.dir, sizeof(mosaic.dir), "%s/%s", cwd, argv[optind+1]);
    }

    // Test mount point
    int rv;
    struct stat s;
    rv = stat(fuse_argv[1], &s);
    if (rv == 0) {
        // Mount point exists: make sure it is a regular and empty file
        if ( !S_ISREG(s.st_mode) | (s.st_size !=0) ) {
            fprintf(stderr,"Mount point (%s) is not a regular empty file\n",fuse_argv[1]);
            exit(-1);
        }
    } else {
        // Mount point does not exist: create it
        fd = fopen(fuse_argv[1], "w");
        fclose(fd);
    }

    // Read tile size and number of tiles from 'mosaicfs.index' file
    char fpath[PATH_MAX];
    snprintf(fpath, sizeof(fpath), "%s/mosaicfs.index", mosaic.dir);
    fd = fopen(fpath,"r");
    if ( fd != NULL ) {
        // Initialize mosaic struct
        fscanf(fd,"%d %ld %ld", &mosaic.format, &mosaic.tile_size, &mosaic.ntiles);
        if (mosaic.format != FORMAT) {
            fprintf(stderr, "ERROR: image format not supported %d\n",FORMAT);
            exit(-1);
        }
        if (mosaic.ntiles > NTILES_MAX) {
            fprintf(stderr, "ERROR: number of tiles cannot exceed %d\n",NTILES_MAX);
            exit(-1);
        }
        mosaic.size = mosaic.tile_size * mosaic.ntiles;
        mosaic.fd = (int *)malloc(mosaic.ntiles * sizeof(int));
        mosaic.nfiles = 0;
        mosaic.nfiles_max = min(NFILES_MAX, FOPEN_MAX);
        fclose(fd);
    } else {
        fprintf(stderr, "ERROR: could not open mosaicfs.index file\n");
        exit(-1);
    }
        
    // Now on to FUSE
    return fuse_main(fuse_argc, fuse_argv, &mosaicfs_oper, NULL);
};

// MosaicFS create
int create_cmd(int argc, char **argv)
{
    char * dirname;
    dirname = argv[optind+1];

    // Test target dir
    int rv;
    struct stat s;
    rv = stat(dirname, &s);
    if (rv == 0) {
        if ( S_ISDIR(s.st_mode) ) {
            // dir exists: make sure it is an empty directory
           int n = 0;
           struct dirent *d;
           DIR *dir = opendir(dirname);
           while ((d = readdir(dir)) != NULL) {
             if (++n > 2)
               break;
           }
           closedir(dir);
           if (n > 2) {
               fprintf(stderr,"Image directory (%s) is not empty\n",dirname);
               exit(-1);
           }
        } else {
            fprintf(stderr,"Image directory (%s) is not an empty directory\n",dirname);
            exit(-1);
        }
    } else {
        // dir does not exist: create it
        mkdir(dirname,S_IRWXU);
    }

    // Create mosaicfs.index file
    FILE* fd;
    char fpath[PATH_MAX];
    snprintf(fpath, sizeof(fpath), "%s/mosaicfs.index", dirname);
    fd = fopen(fpath,"w");
    if ( fd != NULL ) {
        // Initialize mosaic struct
        fprintf(fd,"%d %ld %ld\n", FORMAT, args.size, args.number);
        fclose(fd);
    } else {
        fprintf(stderr, "ERROR: could not create mosaicfs.index file\n");
        exit(-1);
    }
 
    return 0;
};

// Main
int main(int argc, char **argv)
{

   int r;

    // --- parse command line arguments ---
    parse(argc, argv);

    // --- mosaicfs mount subcommand ---
    if (args.mount) {
        r = mount_cmd(argc, argv);
    }

    // --- mosaicfs create subcommand ---
    if (args.create) {
        r = create_cmd(argc, argv);
    }

    return r;
}
