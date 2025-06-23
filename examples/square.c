#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define GENERATOR_IMPLEMENTATION
#include "generator.h"

void* square(void *arg)
{
    (void)arg;
    long y = 0;
    while (y >= 0) {
        y = (long) yield(y*y);
    }

    return NULL;
}

int main(void)
{
    void* base = malloc(4096);

    Generator g = generator_create(square, NULL, base, 4096);
    for (long x = 0; x < 100; ++x) {
        long xx = (long) next_send(&g, x);
        printf("%ld\n", xx);
    }
    next_send(&g, -1);

    free(generator_destroy(&g));
    return 0;
}
