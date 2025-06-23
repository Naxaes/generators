#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GENERATOR_IMPLEMENTATION
#include "generator.h"

void* fib(void *arg)
{
    const long max = (long) arg;
    long a = 0;
    long b = 1;
    while (a < max) {
        const long i = (long) yield(a);
        const long c = a + b;
        a = b;
        b = c;
        printf("fib(%ld) = %ld\n", i, a);
    }

    return (void*) 123;
}

int main(void)
{
    next(g_current_generator_context);

    char* base = malloc(1024);

    memset(base + 0 * 256, 0xaa, 256);
    memset(base + 1 * 256, 0xbb, 256);
    memset(base + 2 * 256, 0xcc, 256);
    memset(base + 3 * 256, 0xdd, 256);

    Generator g = generator_create(fib, (void*)25, base + 2*256, 256);
    long i = 0;
    for (void* value = next(&g); has_next(g); value = next_send(&g, i++)) {
        printf("%ld\n", (long)value);
    }


    long ret = (long) generator_return_value(g);
    void* stack = generator_destroy(&g);
    printf("Return value: %ld\n", ret);

    assert(stack == base + 2*256);
    free(stack);
}
