#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "features.h"

static int have_rdrand(void)
{
#ifdef HAVE_RDRAND
	uint32_t regs[4];
#ifndef __x86_64__
	uint32_t a, b;
	__asm__("pushf\n\t"
		"pushf\n\t"
		"pop %0\n\t"
		"mov %0, %1\n\t"
		"xor %2, %0\n\t"
		"push %0\n\t"
		"popf\n\t"
		"pushf\n\t"
		"pop %0\n\t"
		"popf\n\t"
		: "=&r"(a), "=&r"(b)
		: "i"(0x00200000));
	if(!((a ^ b) & 0x00200000))
		return 0;
#endif

#if defined(__x86_64__) || !defined(__PIC__)
	__asm__("cpuid\n\t"
		: "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
		: "a"(1));
#else
	__asm__("xchg %%ebx, %k1\n\t"
		"cpuid\n\t"
		"xchg %%ebx, %k1\n\t"
		: "=a"(regs[0]), "=&r"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
		: "a"(1));
#endif
	if(regs[2] & 0x40000000) return 1;
#endif
	return 0;
}

int getentropy_rdrand(void *buf, size_t len)
{
	uint32_t r;
	uint8_t err;
	size_t m;
	int i;
	if(!have_rdrand()) {
		errno = ENOSYS;
		return -1;
	}
#ifdef HAVE_RDRAND
	while(len) {
		m = len > 4 ? sizeof(r) : len;
		err = 0;
		for(i = 0; i < 10 && !err; i++) {
			__asm__("rdrand %0\n\t"
				"setc %1\n\t"
				: "=r"(r), "=qm"(err));
		}
		if(err) {
			errno = EIO;
			return -1;
		}
		memcpy(buf, &r, m);
		len -= m;
		buf += m;
	}
	return 0;
#endif
	errno = ENOSYS;
	return -1;
}
