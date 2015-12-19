#include <stddef.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/sysctl.h>
#include <linux/random.h>

int main()
{
	int mib[] = { CTL_KERN, KERN_RANDOM, RANDOM_UUID };

	struct __sysctl_args args = {
		.name = mib,
		.nlen = 3,
		.oldval = (char[16]){},
		.oldlenp = (size_t[]){16}
	};
	syscall(SYS__sysctl, &args);
}
