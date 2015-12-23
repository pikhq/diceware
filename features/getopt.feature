#include <unistd.h>

int main()
{
	int (*p)(int, char*const[], const char*) = getopt;
	optarg = 0;
	optind = opterr = optopt = 0;
	return 0;
}
