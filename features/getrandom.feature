#include <stdlib.h>
#include <linux/random.h>
int main(void)
{
	int (*p)(void*,size_t,unsigned) = getrandom;
	p(0,0,GRND_RANDOM);
	getrandom(0,0,GRND_RANDOM);
}
