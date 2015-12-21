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
#include <signal.h>

/* Be advised: this cutdown implementation is neither thread-safe nor does it
 * handle regeneration of random numbers on fork. This is not a detriment in
 * this application but it might be in yours! Be careful.
 */

int getentropy(void*, size_t);

#define KEYSZ 32
#define IVSZ 8
#define BLOCKSZ 64
#define RSBUFSZ (16*BLOCKSZ)

struct chacha_ctx {
	uint32_t input[16];
};

static uint32_t rotate(uint32_t x, uint32_t y)
{
	return (x << y) | (x >> (32 - y));
}

static void quarterround(uint32_t x[16], int a, int b, int c, int d)
{
	x[a] += x[b];
	x[d] = rotate(x[d]^x[a], 16);
	x[c] += x[d];
	x[b] = rotate(x[b]^x[c], 12);
	x[a] += x[b];
	x[d] = rotate(x[d]^x[a], 8);
	x[c] += x[d];
	x[b] = rotate(x[b]^x[c], 7);
}

static uint32_t get_uint32(const uint8_t x[4])
{
	return x[0] | x[1] << 8 | x[2] << 16 | x[3] << 24;
}

static void write_uint32(uint32_t x, uint8_t y[4])
{
	y[0] = x;
	y[1] = x >> 8;
	y[2] = x >> 16;
	y[3] = x >> 24;
}

static void chacha_keysetup(struct chacha_ctx *x, const uint8_t *k)
{
	const uint8_t *iv = k + KEYSZ;
	static const uint8_t sigma[16] = "expand 32-byte k";
	int i;

	for(i = 0; i < 4; i++)
		x->input[i] = get_uint32(&sigma[i*4]);
	for(i = 0; i < 8; i++)
		x->input[i+4] = get_uint32(&k[i*4]);
	for(i = 0; i < 2; i++)
		x->input[i+12] = 0;
	for(i = 0; i < 2; i++)
		x->input[i+14] = get_uint32(&iv[i*4]);
}

static void chacha_keystream(struct chacha_ctx *ctx, uint8_t *c, size_t bytes)
{
	uint8_t output[64];
	uint32_t x[16];
	int i, j;

	if(!bytes)return;
	for(;;) {
		for(i = 0; i < 16; i++) x[i] = ctx->input[i];
		for(i = 8; i > 0; i -= 2) {
			for(j = 0; j < 4; j++)
				quarterround(x, j, j+4, j+8, j+12);
			for(j = 0; j < 4; j++)
				quarterround(x, j, ((j+1)%4)+4, ((j+2)%4)+8, ((j+3)%4)+12);
		}
		for(i = 0; i < 16; i++) x[i] = x[i] + ctx->input[i];
		for(i = 0; i < 16; i++) write_uint32(x[i], &output[4*i]);
		ctx->input[12]++;
		if(!ctx->input[12]) {
			ctx->input[13]++;
		}
		if(bytes <= 64) {
			for(i = 0; i < bytes; i++) c[i] = output[i];
			return;
		}
		for(i = 0; i < 64; i++) c[i] = output[i];
		bytes -= 64;
		c += 64;
	}
}

static struct chacha_ctx chacha;

static void init(void)
{
	unsigned char rnd[KEYSZ + IVSZ];

	if(getentropy(rnd, sizeof rnd) == -1)
		raise(SIGKILL);
	chacha_keysetup(&chacha, rnd);
}

void posix_random_buffer(void *buf, size_t n)
{
	static int inited = 0;
	static uint16_t refresh_count;
	static unsigned char ks_buf[RSBUFSZ];
	static uint32_t have;
	size_t m;
	if(!inited) {
		init();
		inited = 1;
	}

	while(n > 0) {
		if(have > 0) {
			unsigned char *keystream;
			m = n < have ? n : have;
			keystream = ks_buf + sizeof(ks_buf) - have;
			memcpy(buf, keystream, m);
			memset(keystream, 0, m);
			buf += m;
			n -= m;
			have -= m;
		}
		if(have == 0) {
			chacha_keystream(&chacha, ks_buf, sizeof(ks_buf));
			refresh_count++;
			if(refresh_count == (1600000/RSBUFSZ)) {
				unsigned char rnd[KEYSZ + IVSZ];
				int i;
				if(getentropy(rnd, sizeof rnd) != -1) {
					for(i = 0; i < KEYSZ + IVSZ; i++)
						ks_buf[i] ^= rnd[i];
				}
				refresh_count = 0;
			}
			chacha_keysetup(&chacha, ks_buf);
			memset(ks_buf, 0, KEYSZ + IVSZ);
			have = sizeof(ks_buf) - KEYSZ - IVSZ;
		}
	}
}

uint32_t posix_random(void)
{
	uint32_t val;
	posix_random_buffer(&val, sizeof(val));
	return val;
}

uint32_t posix_random_uniform(uint32_t up)
{
	uint32_t tmp, min;

	if(up < 2) return 0;
	min = -up % up;
	for(;;) {
		tmp = posix_random();
		if(tmp >= min) break;
	}
	return tmp % up;
}

#endif

#endif
