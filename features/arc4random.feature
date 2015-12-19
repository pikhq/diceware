#include <stdlib.h>
#include <stdint.h>
int main()
{
	uint32_t(*p)(void) = arc4random;
	(void)p();
	(void)arc4random();
}
