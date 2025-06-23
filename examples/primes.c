#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define GENERATOR_IMPLEMENTATION
#include "generator.h"

void* primes(void *arg)
{
    long up_to = arg ? (long) arg : LONG_MAX;

    yield(2);

    for (long n = 3; n < up_to; n += 2) {
        int stack_size = 4096;
        void* stack = malloc(stack_size);

        Generator primes_gen = generator_create(primes, NULL, stack, stack_size);

        bool is_prime = true;
        foreach (value, &primes_gen) {
            long p = (long)value;

            if (p * p > n)
                break;

            if (n % p == 0) {
                is_prime = false;
                break;
            }
        }

        free(generator_destroy(&primes_gen));

        if (is_prime) yield(n);
    }

    return NULL;
}


int main(void)
{
    int stack_size = 4096;
    void* stack = malloc(stack_size);
    Generator g = generator_create(primes, (void*) 1000, stack, stack_size);
    foreach (value, &g) {
        printf("%ld\n", (long)value);
    }

    free(generator_destroy(&g));
}
