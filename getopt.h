#include "features.h"
#ifndef HAVE_GETOPT
int getopt(int, char*const[],const char*);
extern char *optarg;
extern int optind, opterr, optopt;
#else
#include <unistd.h>
#endif
