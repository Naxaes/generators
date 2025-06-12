#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __APPLE__
    #define MAP_STACK 0
    #define MAP_GROWSDOWN 0
#endif

#define asm __asm__
#define thread_local _Thread_local

#include <string.h>

#include <sys/mman.h>
#include <unistd.h>

#include "generator.h"

#define GENERATOR_STACK_CAPACITY (1024*getpagesize())


Generator  g_generator_main = { 0 };
Generator* g_generator_stack = &g_generator_main;

#define GENERATOR_EXHAUSTED   ((void*)NULL)
#define GENERATOR_NOT_STARTED ((void*)1)


void return_from_current_generator(void);
Generator generator_create(void (*f)(void*))
{
    void* stack_base = mmap(NULL, GENERATOR_STACK_CAPACITY, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_STACK|MAP_ANONYMOUS|MAP_GROWSDOWN, -1, 0);
    if (stack_base == MAP_FAILED) {
        return g_generator_main;
    }

    void **rsp = (void**)((char*)stack_base + GENERATOR_STACK_CAPACITY);
    *(--rsp) = return_from_current_generator;
    *(--rsp) = f;
    *(--rsp) = 0;   // push rdi
    *(--rsp) = 0;   // push rbx
    *(--rsp) = 0;   // push rbp
    *(--rsp) = 0;   // push r12
    *(--rsp) = 0;   // push r13
    *(--rsp) = 0;   // push r14
    *(--rsp) = 0;   // push r15

    return (Generator) {
        .rsp = rsp,
        .stack_base = stack_base,
        .parent = GENERATOR_NOT_STARTED,
    };
}

void generator_destroy(Generator *g)
{
    munmap(g->stack_base, GENERATOR_STACK_CAPACITY);
    memset(g, 0, sizeof(*g));
}

bool generator_is_invalid(Generator g) {
    return g.stack_base == NULL;
}

bool generator_is_exhausted(Generator g) {
    return g.parent == GENERATOR_EXHAUSTED;
}

bool generator_is_running(Generator g) {
    return (uintptr_t)g.parent > (uintptr_t)GENERATOR_NOT_STARTED;
}

// Linux x86_64 call convention
// %rdi, %rsi, %rdx, %rcx, %r8, and %r9
void* __attribute__((naked)) generator_next(__attribute__((unused)) Generator *g, __attribute__((unused)) void *arg)
{
    // @arch
    asm(
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, %rdx\n"     // rsp
#ifdef __clang__
    "    jmp _switch_to_generator\n"
#else
    "    jmp switch_to_generator\n"
#endif
    );
}

void *__attribute__((naked)) generator_yield(__attribute__((unused)) void *arg)
{
    // @arch
    asm(
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, %rsi\n"     // rsp
#ifdef __clang__
    "    jmp _yield_from_current_generator\n"
#else
    "    jmp yield_from_current_generator\n"
#endif
    );
}

void __attribute__((naked)) restore_generator_context(__attribute__((unused)) void *rsp, __attribute__((unused)) void *arg)
{
    // @arch
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
    "    ret\n");
}

void* switch_to_generator(Generator* g, void *arg, void *rsp)
{
    if (generator_is_running(*g)) {
        g_generator_stack->rsp = rsp;
        g_generator_stack = g;
        restore_generator_context(g->rsp, arg);
    } else if (generator_is_exhausted(*g)) {
        restore_generator_context(rsp, arg);
        return NULL;  // Unreachable
    } else {
        g_generator_stack->rsp = rsp;
        g->parent = g_generator_stack;
        g_generator_stack = g;

        void **sp = (void**)((char*)g->stack_base + GENERATOR_STACK_CAPACITY);
        *(sp-3) = arg;

        restore_generator_context(g->rsp, arg);
    }
    return NULL;  // Unreachable
}

void yield_from_current_generator(void *arg, void *rsp)
{
    assert(g_generator_stack->parent != NULL);
    g_generator_stack->rsp = rsp;
    g_generator_stack = g_generator_stack->parent;
    restore_generator_context(g_generator_stack->rsp, arg);
}

void return_from_current_generator(void)
{
    Generator* g = g_generator_stack;
    g_generator_stack = g->parent;
    g->parent = GENERATOR_EXHAUSTED;
    restore_generator_context(g_generator_stack->rsp, NULL);
}

