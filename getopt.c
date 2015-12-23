#include "features.h"

#ifndef HAVE_GETOPT

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

char *optarg;
int optind=1, opterr=1, optopt;
static int optpos;

static void getopt_msg(const char *a, const char *b, const char *c, size_t l)
{
	FILE *f = stderr;
	// b = __lctrans_cur(b);
	fwrite(a, strlen(a), 1, f)
	&& fwrite(b, strlen(b), 1, f)
	&& fwrite(c, l, 1, f)
	&& putc('\n', f);
}

int getopt(int argc, char * const argv[], const char *optstring)
{
	int i;
	wchar_t c, d;
	int k, l;
	char *optchar;
	mbstate_t ps;

	memset(&ps, 0, sizeof(ps));

	if (!optind) {
		optpos = 0;
		optind = 1;
	}

	if (optind >= argc || !argv[optind])
		return -1;

	if (argv[optind][0] != '-') {
		if (optstring[0] == '-') {
			optarg = argv[optind++];
			return 1;
		}
		return -1;
	}

	if (!argv[optind][1])
		return -1;

	if (argv[optind][1] == '-' && !argv[optind][2])
		return optind++, -1;

	if (!optpos) optpos++;
	if ((k = mbrtowc(&c, argv[optind]+optpos, MB_LEN_MAX, &ps)) < 0) {
		k = 1;
		c = 0xfffd; /* replacement char */
		memset(&ps, 0, sizeof(ps));
	}
	optchar = argv[optind]+optpos;
	optopt = c;
	optpos += k;

	if (!argv[optind][optpos]) {
		optind++;
		optpos = 0;
	}

	if (optstring[0] == '-' || optstring[0] == '+')
		optstring++;

	i = 0;
	d = 0;
	do {
		l = mbrtowc(&d, optstring+i, MB_LEN_MAX, &ps);
		if (l>0) i+=l; else i++;
	} while (l && d != c);

	if (d != c) {
		if (optstring[0] != ':' && opterr)
			getopt_msg(argv[0], ": unrecognized option: ", optchar, k);
		return '?';
	}
	if (optstring[i] == ':') {
		if (optstring[i+1] == ':') optarg = 0;
		else if (optind >= argc) {
			if (optstring[0] == ':') return ':';
			if (opterr) getopt_msg(argv[0],
				": option requires an argument: ",
				optchar, k);
			return '?';
		}
		if (optstring[i+1] != ':' || optpos) {
			optarg = argv[optind++] + optpos;
			optpos = 0;
		}
	}
	return c;
}
#endif
