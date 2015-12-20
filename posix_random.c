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

#define ROTATE(v, c) ((uint32_t)((v) << (c)) | ((uint32_t)(v) >> (32 - (c))))
#define QUARTERROUND(a,b,c,d) \
	x[a] = x[a] + x[b]; x[d] = ROTATE(x[d]^x[a], 16); \
	x[c] = x[c] + x[d]; x[b] = ROTATE(x[b]^x[c], 12); \
	x[a] = x[a] + x[b]; x[d] = ROTATE(x[d]^x[a], 8); \
	x[c] = x[c] + x[d]; x[b] = ROTATE(x[b]^x[c], 7);

static void chacha_keysetup(struct chacha_ctx *x, const uint8_t *k)
{
	const uint8_t *iv = k + KEYSZ;
	static const uint8_t sigma[16] = "expand 32-byte k";

	x->input[4] = k[0] | k[1] << 8 | k[2] << 16 | k[3] << 24;
	x->input[5] = k[4] | k[5] << 8 | k[6] << 16 | k[7] << 24;
	x->input[6] = k[8] | k[9] << 8 | k[10] << 16 | k[11] << 24;
	x->input[7] = k[12] | k[13] << 8 | k[14] << 16 | k[15] << 24;
	x->input[8] = k[16] | k[17] << 8 | k[18] << 16 | k[19] << 24;
	x->input[9] = k[20] | k[21] << 8 | k[22] << 16 | k[23] << 24;
	x->input[10] = k[24] | k[25] << 8 | k[26] << 16 | k[27] << 24;
	x->input[11] = k[28] | k[29] << 8 | k[30] << 16 | k[31] << 24;
	x->input[0] = sigma[0] | sigma[1] << 8 | sigma[2] << 16 | sigma[3] << 24;
	x->input[1] = sigma[4] | sigma[5] << 8 | sigma[6] << 16 | sigma[7] << 24;
	x->input[2] = sigma[8] | sigma[9] << 8 | sigma[10] << 16 | sigma[11] << 24;
	x->input[3] = sigma[12] | sigma[13] << 8 | sigma[14] << 16 | sigma[15] << 24;
	x->input[12] = 0;
	x->input[13] = 0;
	x->input[14] = iv[0] | iv[1] << 8 | iv[2] << 16 | iv[3] << 24;
	x->input[15] = iv[4] | iv[5] << 8 | iv[6] << 16 | iv[7] << 24;
}

static void chacha_keystream(struct chacha_ctx *ctx, uint8_t *c, size_t bytes)
{
	uint8_t output[64];
	uint32_t x[16];
	int i;

	if(!bytes)return;
	for(; bytes > 0; c += 64, bytes = bytes >= 64 ? bytes - 64 : 0) {
		for(i = 0; i < 16; i++) x[i] = ctx->input[i];
		for(i = 8; i > 0; i -= 2) {
			QUARTERROUND(0, 4, 8, 12);
			QUARTERROUND(1, 5, 9, 13);
			QUARTERROUND(2, 6, 10, 14);
			QUARTERROUND(3, 7, 11, 15);
			QUARTERROUND(0, 5, 10, 15);
			QUARTERROUND(1, 6, 11, 12);
			QUARTERROUND(2, 7, 8, 13);
			QUARTERROUND(3, 4, 9, 14);
		}
		for(i = 0; i < 16; i++) x[i] = x[i] + ctx->input[i];
		for(i = 0; i < 16; i++) {
			output[4*i+0] = x[i];
			output[4*i+1] = x[i] >> 8;
			output[4*i+2] = x[i] >> 16;
			output[4*i+3] = x[i] >> 24;
		}
		ctx->input[12]++;
		if(!ctx->input[12]) {
			ctx->input[13]++;
		}
		for(i = 0; i < bytes && i < 64; i++) c[i] = output[i];
	}
}

static struct chacha_ctx chacha;

static void init(void)
{
	unsigned char rnd[KEYSZ + IVSZ];

	if(getentropy(rnd, sizeof rnd) == -1)
		abort();
	chacha_keysetup(&chacha, rnd);
}

void posix_random_buffer(void *buf_void, size_t n)
{
	static int inited = 0;
	unsigned char *buf = buf_void;
	static unsigned char rsbuf[RSBUFSZ];
	static int i;

	if(!inited) {
		init();
		inited = 1;
	}

	while(n > 0) {
		if(i + n < RSBUFSZ) {
			memcpy(buf, rsbuf + i, n);
			buf += n;
			i += n;
			n = 0;
		} else {
			memcpy(buf, rsbuf + i, RSBUFSZ - i);
			chacha_keystream(&chacha, rsbuf, RSBUFSZ);
			chacha_keysetup(&chacha, rsbuf);
			buf += RSBUFSZ - i;
			i = KEYSZ + IVSZ;;
			n -= RSBUFSZ - i;
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
