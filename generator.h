#ifndef GENERATOR_H_
#define GENERATOR_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct Generator {
    void* stack_base;
    void* stack_ptr;
    void* stack_top;
    struct Generator* parent;
} Generator;

extern Generator* g_current_generator_context;

bool generator_is_exhausted(Generator g);
bool generator_is_running(Generator g);

Generator generator_create(void* (*f)(void*), void* arg, void* stack, size_t stack_size);
void*     generator_switch(Generator *g, void *arg);
void*     generator_return_value(Generator g);
void*     generator_destroy(Generator *g);

#define generator_next(g)           generator_switch(g,     NULL)
#define generator_next_send(g, arg) generator_switch(g,    (void*)(arg))
#define generator_yield(arg)        generator_switch(NULL, (void*)(arg))

#define foreach(it, g)            for (void* it = next(g); has_next(*(g)); it = next(g))
#define foreach_send(it, g, val)  for (void* it = next(g); has_next(*(g)); it = next_send(g, val))

#define has_next(g)       (!generator_is_exhausted(g))
#define next(g)           generator_next(g)
#define next_send(g, arg) generator_next_send(g, arg)
#define yield(arg)        generator_yield(arg)


#endif // GENERATOR_H_

#define  GENERATOR_IMPLEMENTATION
#ifdef GENERATOR_IMPLEMENTATION

#include <stdint.h>

#include <assert.h>

#define asm __asm__
#define thread_local _Thread_local


Generator  g_generator_main = { 0 };
Generator* g_current_generator_context = &g_generator_main;

#define GENERATOR_EXHAUSTED   ((void*)NULL)
#define GENERATOR_NOT_STARTED ((void*)1)

void return_from_current_generator(void);
Generator generator_create(void* (*f)(void*), void* arg, void* stack, size_t stack_size)
{
    assert(stack_size >= 16*sizeof(void*) + 16*sizeof(void*));
    // TODO: Align stack pointer to 16 bytes

    void*  stack_top = (char*)stack + stack_size;
    void** ptr = stack_top;
    *(--ptr) = (void*) 0xdeadbeefcafebabe;       // Return value
    *(--ptr) = (void*) return_from_current_generator;
    *(--ptr) = (void*) f;
    *(--ptr) = arg;     // push rdi
    *(--ptr) = 0;       // push rbx
    *(--ptr) = 0;       // push rbp
    *(--ptr) = 0;       // push r12
    *(--ptr) = 0;       // push r13
    *(--ptr) = 0;       // push r14
    *(--ptr) = 0;       // push r15

    return (Generator) {
        .stack_base = stack,
        .stack_ptr = ptr,
        .stack_top = stack_top,
        .parent = GENERATOR_NOT_STARTED,
    };
}

void* generator_return_value(Generator g)
{
    assert(generator_is_exhausted(g));
    return *((void**)g.stack_top - 1);
}

void* generator_destroy(Generator *g)
{
    void* base = g->stack_base;
    *g = (Generator) { 0 };
    return base;
}

bool generator_is_exhausted(Generator g) {
    return g.parent == GENERATOR_EXHAUSTED;
}

bool generator_is_running(Generator g) {
    return (uintptr_t)g.parent > (uintptr_t)GENERATOR_NOT_STARTED;
}

// Linux x86_64 call convention
// rdi, rsi, rdx, rcx, r8, and r9 are arguments
// r12, r13, r14, r15, rbx, rsp, rbp are the callee-saved registers
void* __attribute__((naked)) generator_switch(__attribute__((unused)) Generator *g, __attribute__((unused)) void *arg)
{
    // @arch - Push the `arg` on the stack and then all callee-saved registers. Then jump to `switch_context`.
    asm(
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, %rdx\n"     // rsp
    "    jmp switch_context\n"
    );
}

void __attribute__((naked)) restore_context(__attribute__((unused)) void *rsp, __attribute__((unused)) void *arg)
{
    // @arch - Set the stack to `rsp` and prepare the return value `arg`. Then restore all callee-saved registers before returning.
    asm(
    "    movq %rdi, %rsp\n"     // Set stack pointer to the new stack
    "    movq %rsi, %rax\n"     // Set return value (for `generator_next` and `generator_yield`)
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rbx\n"
    "    popq %rbp\n"
    "    popq %rdi\n"
    "    ret\n"
    );
}

extern void switch_context(Generator* g, void *arg, void *rsp) asm("switch_context");
void switch_context(Generator* g, void *arg, void *rsp)
{
    // Set current context rsp
    g_current_generator_context->stack_ptr = rsp;

    if (g == NULL) {
        // yield
        assert(g_current_generator_context->parent != NULL);
        g_current_generator_context = g_current_generator_context->parent;
        restore_context(g_current_generator_context->stack_ptr, arg);
    } else if (generator_is_running(*g)) {
        // next / next_send  -  on running generator
        g_current_generator_context = g;
        restore_context(g->stack_ptr, arg);
    } else if (generator_is_exhausted(*g)) {
        // next / next_send  -  on exhausted generator
        restore_context(rsp, arg);
    } else {
        // next / next_send  -  on newly created generator
        g->parent = g_current_generator_context;
        g_current_generator_context = g;
        restore_context(g->stack_ptr, arg);
    }
}

void return_from_current_generator(void)
{
    void* return_value = 0;
    __asm__ volatile ("movq %%rax, %0\n": "=r"(return_value));
    Generator* g = g_current_generator_context;
    *((void**)g->stack_top - 1) = return_value;
    g_current_generator_context = g->parent;
    g->parent = GENERATOR_EXHAUSTED;
    restore_context(g_current_generator_context->stack_ptr, NULL);
}




#endif
