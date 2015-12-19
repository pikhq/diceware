#include <sys/ioctl.h>
#include <linux/random.h>

int main()
{
	int cnt;
	ioctl(0, RNDGETENTCNT, &cnt);
}
