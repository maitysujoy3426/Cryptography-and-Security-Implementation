// chacha20_alt2.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <x86intrin.h> // __rdtsc()

typedef uint8_t  u8;
typedef uint32_t u32;

/* 32-bit left rotate */
static inline u32 rotl(u32 v, int n) { return (v << n) | (v >> (32 - n)); }

/* quarter round macro (same operations and order as original) */
#define QR(a,b,c,d) do { \
    a += b; d ^= a; d = rotl(d,16); \
    c += d; b ^= c; b = rotl(b,12); \
    a += b; d ^= a; d = rotl(d,8);  \
    c += d; b ^= c; b = rotl(b,7);  \
} while(0)

/* print 4x4 state words as 8-digit hex per word */
static void print_state(const u32 s[16]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            printf("%08x ", s[r*4 + c]);
        }
        printf("\n");
    }
    printf("\n");
}

/* load 32-bit word from 4 bytes in little-endian */
static inline u32 load32_le(const u8 *b) {
    return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
}

/* store 32-bit word into 4 bytes little-endian */
static inline void store32_le(u8 *b, u32 v) {
    b[0] = (u8)v;
    b[1] = (u8)(v >> 8);
    b[2] = (u8)(v >> 16);
    b[3] = (u8)(v >> 24);
}

/* The ChaCha20 block function — prints initial/state/output same as original */
void chacha20_block(u32 out[16], const u32 in[16]) {
    u32 state[16];
    for (int i = 0; i < 16; ++i) state[i] = in[i];

    printf("Initial state:\n");
    print_state(state);

    /* 20 rounds -> 10 double-rounds */
    for (int i = 0; i < 10; ++i) {
        /* column rounds */
        QR(state[0], state[4], state[8],  state[12]);
        QR(state[1], state[5], state[9],  state[13]);
        QR(state[2], state[6], state[10], state[14]);
        QR(state[3], state[7], state[11], state[15]);
        /* diagonal/row rounds */
        QR(state[0], state[5], state[10], state[15]);
        QR(state[1], state[6], state[11], state[12]);
        QR(state[2], state[7], state[8],  state[13]);
        QR(state[3], state[4], state[9],  state[14]);
    }

    printf("State after 20 rounds:\n");
    print_state(state);

    for (int i = 0; i < 16; ++i) out[i] = state[i] + in[i];

    printf("Output after adding state with input:\n");
    print_state(out);
}

/* initialize chacha20 state from key (32), nonce (12) and counter */
void initialize_state(u32 st[16], const u8 key[32], const u8 nonce[12], u32 counter) {
    st[0] = 0x61707865; st[1] = 0x3320646e; st[2] = 0x79622d32; st[3] = 0x6b206574;
    for (int i = 0; i < 8; ++i) st[4 + i] = load32_le(key + 4*i);
    st[12] = counter;
    st[13] = load32_le(nonce + 0);
    st[14] = load32_le(nonce + 4);
    st[15] = load32_le(nonce + 8);
}

/* encrypt in-place: plaintext -> ciphertext (separate buffers) */
void chacha20_encrypt(const u8 *pt, u8 *ct, size_t len, const u8 key[32], const u8 nonce[12], u32 counter) {
    u32 st[16], ks[16];
    size_t off = 0;

    while (len > 0) {
        initialize_state(st, key, nonce, counter);
        chacha20_block(ks, st);

        /* serialize keystream to bytes (little-endian) */
        u8 keystream[64];
        for (int i = 0; i < 16; ++i) store32_le(keystream + 4*i, ks[i]);

        size_t block = len < 64 ? len : 64;
        for (size_t i = 0; i < block; ++i) ct[off + i] = pt[off + i] ^ keystream[i];

        off += block;
        len -= block;
        ++counter;
    }
}

int main(void) {
    u8 key[32] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f
    };
    u8 nonce[12] = {
        0x00,0x00,0x00,0x09, 0x00,0x00,0x00,0x4a,
        0x00,0x00,0x00,0x00
    };
    u8 plaintext[114] = {
        0x4c,0x61,0x64,0x69,0x65,0x73,0x20,0x61,0x6e,0x64,0x20,0x47,0x65,0x6e,0x74,0x6c,
        0x65,0x6d,0x65,0x6e,0x20,0x6f,0x66,0x20,0x74,0x68,0x65,0x20,0x63,0x6c,0x61,0x73,
        0x73,0x20,0x6f,0x66,0x20,0x27,0x39,0x39,0x3a,0x20,0x49,0x66,0x20,0x49,0x20,0x63,
        0x6f,0x75,0x6c,0x64,0x20,0x6f,0x66,0x66,0x65,0x72,0x20,0x79,0x6f,0x75,0x20,0x6f,
        0x6e,0x6c,0x79,0x20,0x6f,0x6e,0x65,0x20,0x74,0x69,0x70,0x20,0x66,0x6f,0x72,0x20,
        0x74,0x68,0x65,0x20,0x66,0x75,0x74,0x75,0x72,0x65,0x2c,0x20,0x73,0x75,0x6e,0x73,
        0x63,0x72,0x65,0x65,0x6e,0x20,0x77,0x6f,0x75,0x6c,0x64,0x20,0x62,0x65,0x20,0x69,
        0x74,0x2e
    };

    size_t len = sizeof plaintext;
    u8 ciphertext[sizeof plaintext];
    u8 decrypted[sizeof plaintext];

    unsigned long long min_cycles = ULLONG_MAX, max_cycles = 0, total_cycles = 0;
    unsigned long long start, end;
    int trials = 1;

    for (int t = 0; t < trials; ++t) {
        start = __rdtsc();
        chacha20_encrypt(plaintext, ciphertext, len, key, nonce, 1);
        chacha20_encrypt(ciphertext, decrypted, len, key, nonce, 1);
        end = __rdtsc();
        unsigned long long cyc = end - start;
        if (cyc < min_cycles) min_cycles = cyc;
        if (cyc > max_cycles) max_cycles = cyc;
        total_cycles += cyc;
    }

    double avg_cycles = (double)total_cycles / (double)trials;

    /* Print results in same order/format as original */
    printf("Plaintext:  %s\n", (char*)plaintext);

    printf("Ciphertext (hex): ");
    for (size_t i = 0; i < len; ++i) printf("%02x ", ciphertext[i]);
    printf("\n");

    printf("Decrypted:  %s\n", (char*)decrypted);

    if (memcmp(plaintext, decrypted, len) == 0)
        printf("Decryption successful: plaintext matches decrypted text.\n");
    else
        printf("Decryption failed: plaintext does not match decrypted text.\n");

    printf("Average clock cycles: %.2f\n", avg_cycles);
    printf("Minimum clock cycles: %llu\n", min_cycles);
    printf("Maximum clock cycles: %llu\n", max_cycles);

    return 0;
}
