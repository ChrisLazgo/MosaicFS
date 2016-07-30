#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

struct args {
    int create;
    int mount;
    long int size;
    long int number;
    char *fuse;
};

void parse( int argc, char *argv[] );
