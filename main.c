#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "posix_random.h"

static const char* get_word(int n) {
	static const char dicewds[] = 
#include "diceware8k.h"
	"\0";
	const char *p;

	for(p = dicewds; *p && n; n--, p += strlen(p) + 1);
	return p;
}

int main(int argc, char **argv)
{
	int num, i, c, opt;
	int len;
	char **v;

	const char *list = 0;

	int use_rand = 0;
	int add_rand = 0;

	while((opt = getopt(argc, argv, "DARdsl:L:")) > 0) {
		switch(opt) {
		case 'D':
			list = "0123456789";
			use_rand = 1;
			break;
		case 'd':
			list = "0123456789";
			add_rand = 1;
			break;
		case 's':
			list = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-=+[]{}\\|`;:'\"<>/?.,~_";
			add_rand = 1;
			break;
		case 'A':
			list = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
			use_rand = 1;
			break;
		case 'R':
			list = 
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-=+[]{}\\|`;:'\"<>/?.,~_";
			use_rand = 1;
			break;
		case 'l':
			list = optarg;
			add_rand = 1;
			break;
		case 'L':
			list = optarg;
			use_rand = 1;
			break;
		default:
			return 1;
		}
	}

	c = argc - optind;
	v = argv + optind;

	if(c == 0) {
		c = 1;
		v = (char*[]){"6", 0};
	}
	
	num = atoi(*v);
	if(num <= 0) return 0;

	if(use_rand) {
		for(i = 0; i < num; i++)
			printf("%c", list[posix_random_uniform(strlen(list))]);
		printf("\n");
		return 0;
	}

	char **strs = calloc(num, sizeof(*strs));
	if(!strs) abort();

retry:
	len = 0;

	for(i = 0; i < num; i++) {
		int roll = posix_random_uniform(8192);
		strs[i] = strdup(get_word(roll));
		if(!strs[i]) abort();
	}

	if(add_rand) {
		int roll1, roll2, roll3;
		roll1 = posix_random_uniform(num);
		roll2 = posix_random_uniform(strlen(strs[roll1]));
		roll3 = posix_random_uniform(strlen(list));
		strs[roll1][roll2] = list[roll3];
	}

	for(i = 0; i < num; i++) {
		len += strlen(strs[i]) + (i+1 < num ? 1 : 0);
	}
	if(len < 17 && num >= 5) goto retry;

	for(i = 0; i < num; i++) {
		printf("%s%c", strs[i], i+1 < num ? ' ' : '\n');
	}
}
