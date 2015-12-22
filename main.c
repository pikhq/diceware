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

	int add_special = 0;
	int add_digit = 0;
	int use_digits = 0;
	int use_alphanum = 0;
	int use_rand = 0;

	while((opt = getopt(argc, argv, "DARds")) > 0) {
		switch(opt) {
		case 'D':
			use_digits = 1;
			use_rand = use_alphanum = 0;
			break;
		case 'd':
			add_digit = 1;
			break;
		case 's':
			add_special = 1;
			break;
		case 'A':
			use_alphanum = 1;
			use_rand = use_digits = 0;
			break;
		case 'R':
			use_rand = 1;
			use_alphanum = use_digits = 0;
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

	if(use_digits) {
		for(i = 0; i < num; i++) {
			printf("%d", posix_random_uniform(10));
		}
		printf("\n");
		return 0;
	} else if(use_alphanum) {
		static const char *alphanum =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		for(i = 0; i < num; i++) {
			printf("%c", alphanum[posix_random_uniform(strlen(alphanum))]);
		}
		printf("\n");
		return 0;
	} else if(use_rand) {
		static const char *rand_str =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-=+[]{}\\|`;:'\"<>/?.,~_";
		for(i = 0; i < num; i++) {
			printf("%c", rand_str[posix_random_uniform(strlen(rand_str))]);
		}
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

	if(add_digit) {
		int roll1, roll2, roll3;
		for(;;) {
			roll1 = posix_random_uniform(num);
			roll2 = posix_random_uniform(strlen(strs[roll1]));

			if(!isdigit(strs[roll1][roll2])) break;
		}
		strs[roll1][roll2] = '0' + posix_random_uniform(10);
	}

	if(add_special) {
		int roll1, roll2, roll3;
		static const char *specials = "!@#$%^&*()-=+[]{}\\|`;:'\"<>/?.,~_";
		for(;;) {
			roll1 = posix_random_uniform(num);
			roll2 = posix_random_uniform(strlen(strs[roll1]));

			if(!strchr(specials, strs[roll1][roll2])) break;
		}
		strs[roll1][roll2] = specials[posix_random_uniform(strlen(specials))];
	}

	for(i = 0; i < num; i++) {
		len += strlen(strs[i]) + (i+1 < num ? 1 : 0);
	}
	if(len < 17 && num >= 5) goto retry;

	for(i = 0; i < num; i++) {
		printf("%s%c", strs[i], i+1 < num ? ' ' : '\n');
	}
}
