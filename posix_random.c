#include "features.h"
#ifndef HAVE_POSIX_RANDOM
#include <stdint.h>

#if defined(HAVE_ARC4_RANDOM)
#include <stdlib.h>

uint32_t posix_random(void)
{
	return arc4random();
}

void posix_random_buffer(void *buf, size_t n)
{
	return arc4random_buf(buf, n);
}

uint32_t posix_random_uniform(uint32_t up)
{
	return arc4random_uniform(up);
}
#else
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define KEYSTREAM_ONLY
#include "chacha_private.h"

int getentropy(void*, size_t);

#define KEYSZ 32
#define IVSZ 8
#define BLOCKSZ 64
#define RSBUFSZ (16*BLOCKSZ)

static struct rs {
	size_t rs_have;
} rs;

static struct rsx {
	chacha_ctx rs_chacha;
	unsigned char rs_buf[RSBUFSZ];
} rsx;

static void rs_rekey(unsigned char *dat, size_t datlen)
{
	chacha_encrypt_bytes(&rsx.rs_chacha, rsx.rs_buf, rsx.rs_buf, sizeof(rsx.rs_buf));
	memset(rsx.rs_buf, 0, KEYSZ + IVSZ);
	rs.rs_have = sizeof(rsx.rs_buf) - KEYSZ - IVSZ;
}

static void rs_random_u32(uint32_t *val)
{
	unsigned char *keystream;
	if(rs.rs_have < sizeof(*val))
		rs_rekey(0, 0);
	keystream = rsx.rs_buf + sizeof(rsx.rs_buf) - rs.rs_have;
	memcpy(val, keystream, sizeof(*val));
	memset(keystream, 0, sizeof(*val));
	rs.rs_have -= sizeof(*val);
}

static void rs_random_buf(void *buf_void, size_t n)
{
	unsigned char *buf = buf_void;
	unsigned char *keystream;
	size_t m;

	while(n > 0) {
		if(rs.rs_have > 0) {
			m = n < rs.rs_have ? n : rs.rs_have;
			keystream = rsx.rs_buf + sizeof(rsx.rs_buf) - rs.rs_have;
			memcpy(buf, keystream, m);
			memset(keystream, 0, m);
			buf += m;
			n -= m;
			rs.rs_have -= m;
		}
		if(rs.rs_have == 0)
			rs_rekey(0, 0);
	}
}

static void do_init(void)
{
	unsigned char rnd[KEYSZ + IVSZ];

	memset(&rsx, 0, sizeof(rsx));
	memset(&rs, 0, sizeof(rs));
	if(getentropy(rnd, sizeof rnd) == -1)
		abort();
	chacha_keysetup(&rsx.rs_chacha, rnd, KEYSZ * 8, 0);
	chacha_ivsetup(&rsx.rs_chacha, rnd + KEYSZ);
	rs.rs_have = 0;
	memset(rsx.rs_buf, 0, sizeof(rsx.rs_buf));
}

static void init(void)
{
	do_init();
	pthread_atfork((void(*)(void))0, (void(*)(void))0, do_init);
}

static pthread_once_t setup_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

uint32_t posix_random(void)
{
	uint32_t val;

	pthread_once(&setup_once, init);
	pthread_mutex_lock(&mtx);
	rs_random_u32(&val);
	pthread_mutex_unlock(&mtx);
	return val;
}

void posix_random_buffer(void *buf, size_t n)
{
	pthread_once(&setup_once, init);
	pthread_mutex_lock(&mtx);
	rs_random_buf(buf, n);
	pthread_mutex_unlock(&mtx);
}

uint32_t posix_random_uniform(uint32_t up)
{
	uint32_t tmp, max;

	max = 0xFFFFFFFF - (0xFFFFFFFF % up);
	
	for(;;) {
		tmp = posix_random();
		if(tmp < max) break;
	}
	return tmp % up;
}

#endif

#endif
