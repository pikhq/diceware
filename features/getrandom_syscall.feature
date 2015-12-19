#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

int main(void)
{
	long (*p)(long,...) = syscall;
	p(SYS_getrandom,(void*)0,(size_t)0,(unsigned)GRND_RANDOM);
	syscall(SYS_getrandom,(void*)0,(size_t)0,(unsigned)GRND_RANDOM);
}
