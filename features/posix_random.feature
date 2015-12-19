#include <stdlib.h>
#include <stdint.h>
int main()
{
	uint32_t(*p)(void) = posix_random;
	(void)p();
	(void)posix_random();
}
