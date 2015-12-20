/* Implementation of arc4random functions initially from OpenBSD.
 *
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 * Copyright (c) 2014, Theo de Raadt <deraadt@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

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

/* Be advised: this cutdown implementation (copied from OpenBSD's) is neither
 * thread-safe nor does it handle regeneration of random numbers on fork. This
 * is not a detriment in this application but it might be in yours! Be careful.
 */

#define KEYSTREAM_ONLY
#include "chacha_private.h"

int getentropy(void*, size_t);

#define KEYSZ 32
#define IVSZ 8
#define BLOCKSZ 64
#define RSBUFSZ (16*BLOCKSZ)

static struct rsx {
	chacha_ctx rs_chacha;
	unsigned char rs_buf[RSBUFSZ];
	size_t rs_have;
} rsx;

static void rs_rekey(void)
{
	chacha_encrypt_bytes(&rsx.rs_chacha, rsx.rs_buf, rsx.rs_buf, sizeof(rsx.rs_buf));
	memset(rsx.rs_buf, 0, KEYSZ + IVSZ);
	rsx.rs_have = sizeof(rsx.rs_buf) - KEYSZ - IVSZ;
}

static void rs_random_buf(void *buf_void, size_t n)
{
	unsigned char *buf = buf_void;
	unsigned char *keystream;
	size_t m;

	while(n > 0) {
		if(rsx.rs_have > 0) {
			m = n < rsx.rs_have ? n : rsx.rs_have;
			keystream = rsx.rs_buf + sizeof(rsx.rs_buf) - rsx.rs_have;
			memcpy(buf, keystream, m);
			memset(keystream, 0, m);
			buf += m;
			n -= m;
			rsx.rs_have -= m;
		}
		if(rsx.rs_have == 0)
			rs_rekey();
	}
}

static void init(void)
{
	unsigned char rnd[KEYSZ + IVSZ];

	memset(&rsx, 0, sizeof(rsx));
	if(getentropy(rnd, sizeof rnd) == -1)
		abort();
	chacha_keysetup(&rsx.rs_chacha, rnd, KEYSZ * 8, 0);
	chacha_ivsetup(&rsx.rs_chacha, rnd + KEYSZ);
	rsx.rs_have = 0;
	memset(rsx.rs_buf, 0, sizeof(rsx.rs_buf));
}
void posix_random_buffer(void *buf, size_t n)
{
	rs_random_buf(buf, n);
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
