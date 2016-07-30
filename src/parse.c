#include "parse.h"
#include "global.h"

extern struct args args;

static void usage(int r)
{
    char help[] =
"Usage: mosaicfs <command> [options]\n"
"\n"
"Available commands and options:\n"
"\n"
"mosaicfs mount <archive> <path> [--fuse=\"<fuse arguments>\"]\n"
"\n"
"   Mount existing MosaicFS archive located in directory <archive> on mount\n"
"   point <path>. <path> will be created if necessary. If <path> exists, it\n"
"   should be an empty file.\n"
"\n"
"   --fuse=\"<...>\" optional mount arguments to be passed to FUSE.\n"
"\n"
"mosaicfs create --number <number of tiles> --size <tile size> <archive>\n"
"\n"
"   Create new MosaicFS archive under directory <archive> with total size\n"
"   <number of tiles> x <tile size>:\n"
"   -n / --number N number of tile files\n"
"   -s / --size   S size of each individual tile file.  Default size is in\n"
"                   bytes, but other units are also available by appending\n"
"                   their appropriate units  (K = kilobyte,  M = megabyte,\n"
"                   G = gigabyte).\n\n";
  
    fprintf(stdout,"%s", help);
    exit(r);
}

void parse( int argc, char *argv[] )
{

    // Make sure a subcommand is present
    if (argc == 1) {
        usage(-1);
    }

   // Define default values
   args.mount = 0;
   args.create = 0;
   args.size = 0;
   args.number = 0;
   args.fuse = NULL;

   // Parse arguments
   static struct option long_options[] =
    {
      {"help",    no_argument,       0, 'h'},
      {"number",  required_argument, 0, 'n'},
      {"size",    required_argument, 0, 's'},
      {"fuse",    required_argument, 0, 'f'},
      {0, 0, 0, 0}
    };
    char *ptr;
    int c;
    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        c = getopt_long (argc, argv, "hn:s:f:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
          break;

        switch (c)
        {
          case 'h':   // -h/--help
            usage(0);

          case 'n':   // -n/--number
            args.number = strtol(optarg, &ptr, 10);
            if (args.number > NTILES_MAX) {
                fprintf(stderr, "ERROR: number of tiles cannot exceed %d\n",NTILES_MAX);
                exit(-1);
            }
            if (ptr[0] != 0)
            {
               printf("Error intepreting -n/--number argument %s\n\n",optarg);
               usage(-1);
            }
            break;

          case 's':   // -s/--size
            args.size = strtol(optarg, &ptr, 10);
            if (ptr[0] != 0)
            {
              switch (ptr[0])
              {
                case 'K':
                case 'k':
                  args.size = args.size * 1024;
                  break;
                case 'M':
                case 'm':
                  args.size = args.size * 1024*1024;
                  break;
                case 'G':
                case 'g':
                  args.size = args.size * 1024*1024*1024;
                  break;
                default:
                  printf("Error intepreting -s/--size argument %s\n\n",optarg);
                  usage(-1);
                  break;
              }
            }
            break;

          case 'f':   // -f/--fuse
            args.fuse = (char *)malloc(sizeof(char)*(strlen(optarg)+1));
            strcpy(args.fuse,optarg);
            break;

          case '?':
            usage(-1);
            break;

          default:
            abort();
        }
    }

    // Make sure a subcommand is present
    if (optind >= argc) {
        usage(-1);
    }

    // Check subcommand
    if (strcmp(argv[optind], "create") == 0) 
    {
      args.create = 1;
      if ( (args.size <= 0) | (args.number <= 0) )
      {
        printf("'create' subcommand requires -n/--number and -s/--size arguments\n\n");
        usage(-1);
      }
 
    }
    else if (strcmp(argv[optind], "mount") == 0)
    {
      args.mount = 1;
    }
    else
    {
      printf("Unknown subcommand \'%s\'\n\n",argv[optind]);
      usage(-1);
    }

    return;
}

