#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <stdbool.h>

typedef struct Generator {
    void* rsp;
    void* const stack_base;
    struct Generator* parent;
} Generator;

bool generator_is_invalid(Generator g);
bool generator_is_exhausted(Generator g);
bool generator_is_running(Generator g);

void* generator_next(Generator *g, void *arg);
void* generator_yield(void *arg);
Generator generator_create(void (*f)(void*));
void generator_destroy(Generator *g);

#define foreach(it, g, arg) for (void *it = generator_next(g, arg); !generator_is_exhausted(*(g)); it = generator_next(g, arg))

#endif // GENERATOR_H_
