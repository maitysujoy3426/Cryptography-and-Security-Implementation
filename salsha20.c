// salsa20_rdtscp.c
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <x86intrin.h>  // RDTSCP / LFENCE

/* 32-bit left rotate */
static inline uint32_t ROTL32(uint32_t v, int n) {
    return (v << n) | (v >> (32 - n));
}

/* Quarter-round as per Salsa20 specification */
static inline void QR(uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    *b ^= ROTL32((*a + *d), 7);
    *c ^= ROTL32((*b + *a), 9);
    *d ^= ROTL32((*c + *b), 13);
    *a ^= ROTL32((*d + *c), 18);
}

/* Salsa20 core block: out = Hash(in) where Hash is 20 rounds + feedforward */
void salsa20_block(uint32_t out[16], const uint32_t in[16]) {
    uint32_t x[16];
    for (int i = 0; i < 16; ++i) x[i] = in[i];

    for (int r = 0; r < 20; r += 2) {
        /* column rounds */
        QR(&x[0], &x[4], &x[8],  &x[12]);
        QR(&x[5], &x[9], &x[13], &x[1]);
        QR(&x[10],&x[14],&x[2],  &x[6]);
        QR(&x[15],&x[3], &x[7],  &x[11]);

        /* row rounds */
        QR(&x[0], &x[1], &x[2],  &x[3]);
        QR(&x[5], &x[6], &x[7],  &x[4]);
        QR(&x[10],&x[11],&x[8],  &x[9]);
        QR(&x[15],&x[12],&x[13], &x[14]);
    }

    for (int i = 0; i < 16; ++i) out[i] = x[i] + in[i];
}

/* Get serialized timestamp counter using RDTSCP with LFENCE for ordering.
   Returns 64-bit cycle count. */
static inline unsigned long long timestamp(void) {
    unsigned int aux;
    _mm_lfence();               // serialize prior instructions
    unsigned long long t = __rdtscp(&aux); // RDTSCP is serializing for later instructions
    _mm_lfence();
    return t;
}

int main(void) {
    uint32_t in[16] = {0};   /* dummy input, as before */
    uint32_t out[16];

    const int runs = 100000;
    unsigned long long min_cycles = ULLONG_MAX;
    unsigned long long max_cycles = 0;
    unsigned long long total_cycles = 0;

    for (int i = 0; i < runs; ++i) {
        unsigned long long start = timestamp();
        salsa20_block(out, in);
        unsigned long long end = timestamp();
        unsigned long long elapsed = end - start;

        if (elapsed < min_cycles) min_cycles = elapsed;
        if (elapsed > max_cycles) max_cycles = elapsed;
        total_cycles += elapsed;
    }

    double avg_cycles = (double)total_cycles / (double)runs;
    printf("Average cycles per run: %.2f\n", avg_cycles);
    printf("Minimum cycles: %llu\n", (unsigned long long)min_cycles);
    printf("Maximum cycles: %llu\n", (unsigned long long)max_cycles);

    (void)out; /* silence unused-variable warnings if any */
    return 0;
}
