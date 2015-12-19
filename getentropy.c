#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "features.h"

#ifndef HAVE_GETENTROPY

#if defined(HAVE_GETRANDOM_SYSCALL) || defined(HAVE_SYSCTL_RANDOM)
#include <sys/syscall.h>
#endif

#if defined(HAVE_GETRANDOM_SYSCALL) || defined(HAVE_LINUX_RANDOM_IOCTL)
#include <linux/random.h>
#endif

#ifdef HAVE_LINUX_RANDOM_IOCTL
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYSCTL_RANDOM
#include <linux/sysctl.h>
#endif

#ifdef HAVE_WIN32_CRYPT
#include <windows.h>
#include <wincrypt.h>
#endif

static int getentropy_win32crypt(void *buf, size_t len)
{
#ifdef HAVE_WIN32_CRYPT
	HCRYPTPROV provider;

	if(CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) == 0) {
		errno = EIO;
		return -1;
	}
	if(CryptGenRandom(provider, len, buf) == 0) {
		CryptReleaseContext(provider, 0);
		errno = EIO;
		return -1;
	}
	CryptReleaseContext(provider, 0);
	return 0;
#endif
	errno = ENOSYS;
	return -1;
}

static int getentropy_getrandom(void *buf, size_t len)
{
#ifdef HAVE_GETRANDOM
	return getrandom(buf, len, 0);
#elif defined(HAVE_GETRANDOM_SYSCALL)
	int ret;
	ret = syscall(SYS_getrandom, buf, len, 0);
	if(ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
#endif
	errno = ENOSYS;
	return -1;
}

// Stupid sanity check; discard buffers without any bits set
static int gotdata(void *buf, size_t len)
{
	uint8_t *b = buf;
	uint8_t any_set;
	size_t i;
	for(i = 0; i < len; i++) {
		any_set |= b[i];
	}
	return any_set ? 0 : -1;
}

static int getentropy_urandom(void *buf, size_t len)
{
	int fd, cnt;
	struct stat st;
	size_t i;

start:
	fd = open("/dev/urandom", O_RDONLY
#ifdef O_CLOEXEC
			| O_CLOEXEC
#endif
			);
	if(fd < 0) {
		if(errno == EINTR) goto start;
		return -1;
	}

	if(fstat(fd, &st) == -1 || !S_ISCHR(st.st_mode)) {
		close(fd);
		errno = EIO;
		return -1;
	}
#ifdef HAVE_LINUX_RANDOM_IOCTL
	if(ioctl(fd, RNDGETENTCNT, &cnt) == -1) {
		if(errno != EINVAL) {
			close(fd);
			errno = EIO;
			return -1;
		}
	}
#endif
	for(i = 0; i < len; i++) {
		ssize_t ret = read(fd, (char*)buf + i, len - i);
		if(ret == -1) {
			int old;
			if(errno == EAGAIN || errno == EINTR) continue;
			old = errno;
			close(fd);
			errno = old;
			return -1;
		}
		i += ret;
	}
	close(fd);
	if(gotdata(buf, len) == 0)
		return 0;
	errno = EIO;
	return -1;
}

static int getentropy_sysctl(void *buf, size_t len)
{
#ifdef HAVE_SYSCTL_RANDOM
	int mib[] = { CTL_KERN, KERN_RANDOM, RANDOM_UUID };
	size_t i, j;

	for(i = 0; i < len; i++) {
		char uuid_buf[16];
		struct __sysctl_args args = {
			.name = mib,
			.nlen = 3,
			.oldval = (char*)buf + i,
			.oldlenp = (size_t[]){16}
		};
		if(syscall(SYS__sysctl, &args) != 0) {
			return -1;
		}
		for(j = 0; j < 16 && j < len; j++) {
			// We discard the non-random UUID bits
			if(j == 6) {
				((char*)buf)[j] = uuid_buf[6] & 0x0F | (uuid_buf[8] & 0x0f << 4);
			} else if(j == 8) {
				continue;
			} else if(j > 8) {
				((char*)buf)[j-1] = uuid_buf[j];
			} else {
				((char*)buf)[j] = uuid_buf[j];
			}
			i++;
		}
	}
	if(gotdata(buf, len) == 0)
		return 0;
	errno = EIO;
	return -1;
#endif
	errno = ENOSYS;
	return -1;
}

int getentropy(void *buf, size_t len)
{
	int ret;
	if(len > 256) {
		errno = EIO;
		return -1;
	}
	ret = getentropy_getrandom(buf, len);
	if(ret != -1) return ret;
	if(errno != ENOSYS) return -1;
	ret = getentropy_win32crypt(buf, len);
	if(ret != -1) return ret;
	ret = getentropy_urandom(buf, len);
	if(ret != -1) return ret;
	ret = getentropy_sysctl(buf, len);
	if(ret != -1) return ret;
	abort();
}

#endif
