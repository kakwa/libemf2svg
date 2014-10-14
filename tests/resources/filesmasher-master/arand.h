#include <stdlib.h>
#include <stdint.h>

int32_t arand_init(unsigned int seed);
int arand_random64(uint64_t *value, int32_t bits);
int arand_random32(uint32_t *value, int32_t bits);
int arand_random16(uint16_t *value, int32_t bits);
int arand_random8(uint8_t *value, int32_t bits);

