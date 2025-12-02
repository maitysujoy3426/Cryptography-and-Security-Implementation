#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <time.h>
#include <x86intrin.h>   // For __rdtsc()

// Always-inline Jacobi
static inline __attribute__((always_inline))
int jacobi_symbol(const mpz_t a, const mpz_t n) {
    return mpz_jacobi(a, n);
}

// Always-inline modular exponentiation
static inline __attribute__((always_inline))
void mod_exp(mpz_t result, const mpz_t base, const mpz_t exp, const mpz_t mod) {
    mpz_powm(result, base, exp, mod);
}

// Solovay–Strassen test
int solovay_strassen(const mpz_t n, int k, gmp_randstate_t rng) {
    // Handle small numbers explicitly
    if (mpz_cmp_ui(n, 2) < 0) return 0;          // n < 2 → composite
    if (mpz_cmp_ui(n, 2) == 0) return 1;         // 2 → prime
    if (mpz_cmp_ui(n, 3) == 0) return 1;         // 3 → prime
    if (mpz_even_p(n)) return 0;                 // even > 2 → composite

    mpz_t a, exp, jac, mod;
    mpz_inits(a, exp, jac, mod, NULL);

    mpz_sub_ui(exp, n, 1);
    mpz_fdiv_q_2exp(exp, exp, 1);  // exp = (n-1)/2

    for (int i = 0; i < k; i++) {
        // Random base a ∈ [2, n-2]
        mpz_urandomm(a, rng, n);
        if (mpz_cmp_ui(a, 2) < 0) mpz_add_ui(a, a, 2);

        // Compute jacobi = (a/n)
        int jacobi = jacobi_symbol(a, n);
        if (jacobi == 0) {
            mpz_clears(a, exp, jac, mod, NULL);
            return 0; // Composite
        }

        // Compute mod = a^((n-1)/2) mod n
        mod_exp(mod, a, exp, n);

        // Bring Jacobi into same ring
        if (jacobi == -1) mpz_sub_ui(jac, n, 1);
        else mpz_set_ui(jac, jacobi);

        if (mpz_cmp(jac, mod) != 0) {
            mpz_clears(a, exp, jac, mod, NULL);
            return 0; // Composite
        }
    }

    mpz_clears(a, exp, jac, mod, NULL);
    return 1; // Probably prime
}

// Generate a probable prime of given bits
void generate_prime(mpz_t prime, int bits, gmp_randstate_t rng) {
    mpz_urandomb(prime, rng, bits);
    mpz_setbit(prime, bits - 1);   // Ensure it's "bits"-bit
    mpz_nextprime(prime, prime);   // Next prime
}

int main() {
    mpz_t n;
    mpz_init(n);
    gmp_randstate_t rng;
    gmp_randinit_default(rng);
    gmp_randseed_ui(rng, time(NULL));

    int choice, k;
    printf("Choose option:\n1. Generate prime (512/768/1024 bits)\n2. Test input number\n> ");
    if (scanf("%d", &choice) != 1) return 1;

    if (choice == 1) {
        int bits;
        printf("Enter bits (512/768/1024): ");
        if (scanf("%d", &bits) != 1) return 1;
        generate_prime(n, bits, rng);
        gmp_printf("Generated %d-bit prime:\n%Zd\n", bits, n);
    } else {
        char input[4096];
        printf("Enter number to test: ");
        if (scanf("%s", input) != 1) return 1;
        mpz_set_str(n, input, 10);
    }

    printf("Enter number of iterations k: ");
    if (scanf("%d", &k) != 1) return 1;

    unsigned long long start = __rdtsc();
    int is_probably_prime = solovay_strassen(n, k, rng);
    unsigned long long end = __rdtsc();

    unsigned long long total_cycles = end - start;
    double avg_cycles = (k > 0) ? ((double) total_cycles / k) : 0.0;

    if (is_probably_prime) {
        printf("Result: PROBABLY PRIME (k=%d, error ≤ 2^-%d)\n", k, k);
    } else {
        printf("Result: COMPOSITE\n");
    }

    printf("Total cycles: %llu\n", total_cycles);
    printf("Average cycles per iteration: %.2f\n", avg_cycles);

    // Cross-check with GMP built-in test
    int gmp_check = mpz_probab_prime_p(n, 25);
    if (gmp_check == 0) printf("[GMP check] Definitely COMPOSITE\n");
    else if (gmp_check == 1) printf("[GMP check] Probably PRIME\n");
    else if (gmp_check == 2) printf("[GMP check] Definitely PRIME\n");

    mpz_clear(n);
    gmp_randclear(rng);
    return 0;
}
