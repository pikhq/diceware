#include <unistd.h>
#include <sys/syscall.h>

#define GRND_NONBLOCK 0x01
#define GRND_RANDOM 0x02

int main(void)
{
	long (*p)(long,...) = syscall;
	p(SYS_getrandom,(void*)0,(size_t)0,(unsigned)GRND_RANDOM);
	syscall(SYS_getrandom,(void*)0,(size_t)0,(unsigned)GRND_RANDOM);
}
