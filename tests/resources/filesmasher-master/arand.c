#include "arand.h"

int32_t __arand_randbits;
uint32_t __arand_randtest;

const uint64_t __arand_bitmasks[] = {
	0x0000000000000000, 0x0000000000000001, 0x0000000000000003, 0x0000000000000007,
	0x000000000000000F, 0x000000000000001F, 0x000000000000003F, 0x000000000000007F,
	0x00000000000000FF, 0x00000000000001FF, 0x00000000000003FF, 0x00000000000007FF,
	0x0000000000000FFF, 0x0000000000001FFF, 0x0000000000003FFF, 0x0000000000007FFF,
	0x000000000000FFFF, 0x000000000001FFFF, 0x000000000003FFFF, 0x000000000007FFFF,
	0x00000000000FFFFF, 0x00000000001FFFFF, 0x00000000003FFFFF, 0x00000000007FFFFF,
	0x0000000000FFFFFF, 0x0000000001FFFFFF, 0x0000000003FFFFFF, 0x0000000007FFFFFF,
	0x000000000FFFFFFF, 0x000000001FFFFFFF, 0x000000003FFFFFFF, 0x000000007FFFFFFF,
	0x00000000FFFFFFFF, 0x00000001FFFFFFFF, 0x00000003FFFFFFFF, 0x00000007FFFFFFFF,
	0x0000000FFFFFFFFF, 0x0000001FFFFFFFFF, 0x0000003FFFFFFFFF, 0x0000007FFFFFFFFF,
	0x000000FFFFFFFFFF, 0x000001FFFFFFFFFF, 0x000003FFFFFFFFFF, 0x000007FFFFFFFFFF,
	0x00000FFFFFFFFFFF, 0x00001FFFFFFFFFFF, 0x00003FFFFFFFFFFF, 0x00007FFFFFFFFFFF,
	0x0000FFFFFFFFFFFF, 0x0001FFFFFFFFFFFF, 0x0003FFFFFFFFFFFF, 0x0007FFFFFFFFFFFF,
	0x000FFFFFFFFFFFFF, 0x001FFFFFFFFFFFFF, 0x003FFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF,
	0x00FFFFFFFFFFFFFF, 0x01FFFFFFFFFFFFFF, 0x03FFFFFFFFFFFFFF, 0x07FFFFFFFFFFFFFF,
	0x0FFFFFFFFFFFFFFF, 0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF,
	0xFFFFFFFFFFFFFFFF
};

int32_t arand_init(unsigned int seed) {
	int32_t i;

	__arand_randtest = 0xFFFFFFFF;
	for(i = 32; i > 0; i--) {
		if((__arand_randtest & RAND_MAX) == __arand_randtest) {
			__arand_randbits = i;
			break;
		}
		__arand_randtest >>= 1;
	}

	srandom(seed);

	return(__arand_randbits);
}

int arand_random64(uint64_t *value, int32_t bits) {
	int32_t bitsleft;

	if(bits < 1 || bits > 64) {
		return(-1);
	}

	bitsleft = bits;

	*value = random();
	bitsleft -= __arand_randbits;
	while(bitsleft > 0) {
		*value |= (random() << (bits - bitsleft));
		bitsleft -= __arand_randbits;
	}

	*value &= __arand_bitmasks[bits];
	return(0);
}

int arand_random32(uint32_t *value, int32_t bits) {
	int32_t bitsleft;

	if(bits < 1 || bits > 32) {
		return(-1);
	}

	bitsleft = bits;

	*value = random();
	bitsleft -= __arand_randbits;
	while(bitsleft > 0) {
		*value |= (random() << (bits - bitsleft));
		bitsleft -= __arand_randbits;
	}

	*value &= __arand_bitmasks[bits];
	return(0);
}

int arand_random16(uint16_t *value, int32_t bits) {
	int32_t bitsleft;

	if(bits < 1 || bits > 16) {
		return(-1);
	}

	bitsleft = bits;

	*value = random();
	bitsleft -= __arand_randbits;
	while(bitsleft > 0) {
		*value |= (random() << (bits - bitsleft));
		bitsleft -= __arand_randbits;
	}

	*value &= __arand_bitmasks[bits];
	return(0);
}

int arand_random8(uint8_t *value, int32_t bits) {
	int32_t bitsleft;

	if(bits < 1 || bits > 8) {
		return(-1);
	}

	bitsleft = bits;

	*value = random();
	bitsleft -= __arand_randbits;
	while(bitsleft > 0) {
		*value |= (random() << (bits - bitsleft));
		bitsleft -= __arand_randbits;
	}

	*value &= __arand_bitmasks[bits];
	return(0);
}
