#ifndef POSIX_RANDOM_H
#define POSIX_RANDOM_H

#include <stdlib.h>
#include <stdint.h>

uint32_t posix_random(void);
void posix_random_buffer(void*, size_t);
uint32_t posix_random_uniform(uint32_t);

#endif
