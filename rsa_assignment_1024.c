#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <x86intrin.h>      // For __rdtsc
#include <gmp.h>            // GMP big integers
#include <string.h>

#define ITERATIONS 1000000    // Use 1000000 for final submission
#define BAR_WIDTH 50        // Width of progress bar

unsigned long long min(unsigned long long a, unsigned long long b) {
    return (a < b) ? a : b;
}
unsigned long long max(unsigned long long a, unsigned long long b) {
    return (a > b) ? a : b;
}

// Generate a random prime of 'bits' length
void generate_prime(mpz_t prime, gmp_randstate_t state, int bits) {
    do {
        mpz_urandomb(prime, state, bits);
        mpz_setbit(prime, bits - 1);
        mpz_nextprime(prime, prime);
    } while (!mpz_probab_prime_p(prime, 25));
}

// Progress bar
void print_progress_bar(int current, int total) {
    float progress = (float)current / total;
    int pos = (int)(BAR_WIDTH * progress);
    printf("[");
    for (int i = 0; i < BAR_WIDTH; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %3d%%\r", (int)(progress * 100));
    fflush(stdout);
}

// Main RSA test function
void rsa_test(int BIT_SIZE) {
    printf("\n\n===============================================\n");
    printf("    RSA TESTING WITH %d-BIT PRIME NUMBERS\n", BIT_SIZE);
    printf("===============================================\n");

    // Initialize GMP random state (Mersenne Twister)
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));
    printf("Pseudo-Random Generator (PRG): GMP Mersenne Twister (gmp_randinit_mt)\n");

    // GMP big integers
    mpz_t p, q, N, phi, e, d, m, c, m_prime;
    mpz_inits(p, q, N, phi, e, d, m, c, m_prime, NULL);
    mpz_t p1, q1;
    mpz_inits(p1, q1, NULL);

    unsigned long long total_cycles = 0, min_cycles = ~0ULL, max_cycles = 0;

    // Prime generation loop with timing
    printf("Generating %d-bit prime pairs for %d iterations...\n", BIT_SIZE, ITERATIONS);
    for (int i = 0; i < ITERATIONS; i++) {
        unsigned long long start = __rdtsc();
        generate_prime(p, state, BIT_SIZE);
        generate_prime(q, state, BIT_SIZE);
        unsigned long long end = __rdtsc();
        unsigned long long cycles = end - start;
        total_cycles += cycles;
        min_cycles = min(min_cycles, cycles);
        max_cycles = max(max_cycles, cycles);
        print_progress_bar(i + 1, ITERATIONS);
    }

    // Output prime generation stats
    printf("\n\n=== Prime Generation Timing for %d-bit Primes ===\n", BIT_SIZE);
    printf("Minimum cycles: %llu\n", min_cycles);
    printf("Maximum cycles: %llu\n", max_cycles);
    printf("Average cycles: %.2f\n", (double)total_cycles / ITERATIONS);

    // Print final primes used
    printf("\nFinal prime p:\n"); gmp_printf("%Zd\n", p);
    printf("\nFinal prime q:\n"); gmp_printf("%Zd\n", q);

    // Step 2a: Compute N = p * q
    unsigned long long start, end;
    start = __rdtsc();
    mpz_mul(N, p, q);
    end = __rdtsc();
    printf("\nStep 2a (N = p × q): %llu cycles\n", end - start);
    printf("RSA Modulus N:\n"); gmp_printf("%Zd\n", N);

    // Step 2b: Compute φ(N) = (p−1)(q−1)
    start = __rdtsc();
    mpz_sub_ui(p1, p, 1);
    mpz_sub_ui(q1, q, 1);
    mpz_mul(phi, p1, q1);
    end = __rdtsc();
    printf("\nStep 2b (φ(N) = (p−1)(q−1)): %llu cycles\n", end - start);
    printf("Euler's Totient φ(N):\n"); gmp_printf("%Zd\n", phi);

    // Step 3: Key generation (e, d)
    start = __rdtsc();
    mpz_set_ui(e, 65537); // common exponent
    if (mpz_invert(d, e, phi) == 0) {
        printf("Error: e has no inverse mod φ(N)\n");
        return;
    }
    end = __rdtsc();
    printf("\nStep 3 (Private key generation): %llu cycles\n", end - start);
    printf("Private key d:\n"); gmp_printf("%Zd\n", d);

    // Step 4a: Generate 1023-bit message
    start = __rdtsc();
    mpz_urandomb(m, state, 1023);
    end = __rdtsc();
    printf("\nStep 4a (Message generation 1023-bit): %llu cycles\n", end - start);
    printf("Original 1023-bit message (m):\n"); gmp_printf("%Zd\n", m);

    // Step 4b: Encryption c = m^e mod N
    start = __rdtsc();
    mpz_powm(c, m, e, N);
    end = __rdtsc();
    printf("\nStep 4b (Encryption): %llu cycles\n", end - start);
    printf("Encrypted message (c):\n"); gmp_printf("%Zd\n", c);

    // Step 4c: Decryption m' = c^d mod N
    start = __rdtsc();
    mpz_powm(m_prime, c, d, N);
    end = __rdtsc();
    printf("\nStep 4c (Decryption): %llu cycles\n", end - start);
    printf("Decrypted message (m'):\n"); gmp_printf("%Zd\n", m_prime);

    // Step 4d: Verify message correctness and time it
    start = __rdtsc();
    int cmp = mpz_cmp(m, m_prime);
    end = __rdtsc();
    printf("\nStep 4d (Message verification): %llu cycles\n", end - start);

    if (cmp == 0)
        printf("Message verification: ✅ SUCCESS\n");
    else
        printf("Message verification: ❌ FAILED\n");

    // Clean up
    mpz_clears(p, q, N, phi, e, d, m, c, m_prime, p1, q1, NULL);
    gmp_randclear(state);
}

int main() {
    rsa_test(1024);   // Run for 512-bit primes
    return 0;
}
