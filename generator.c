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


// Linux x86_64 call convention
// %rdi, %rsi, %rdx, %rcx, %r8, and %r9
void* __attribute__((naked)) generator_next(__attribute__((unused)) Generator *g, __attribute__((unused)) void *arg)
{
    // @arch
    asm(
    "    movb  16(%rdi), %al\n"         // %al = g->dead  (0 or 1)
    "    decb  %al\n"                   // %al = %al - 1
    "    jz    return_with_zero\n"      // if (!c) goto return_with_zero
    "    pushq %rdi\n"
    "    pushq %rbp\n"
    "    pushq %rbx\n"
    "    pushq %r12\n"
    "    pushq %r13\n"
    "    pushq %r14\n"
    "    pushq %r15\n"
    "    movq %rsp, %rdx\n"     // rsp
#ifdef __clang__
    "    jmp _generator_switch_context\n"
#else
    "    jmp generator_switch_context\n"
#endif
    "    return_with_zero:\n"
    "       ret\n"
    );
}

void __attribute__((naked)) generator_restore_context(__attribute__((unused)) void *rsp)
{
    // @arch
    asm(
    "    movq %rdi, %rsp\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rbx\n"
    "    popq %rbp\n"
    "    popq %rdi\n"
    "    ret\n");
}

void __attribute__((naked)) generator_restore_context_with_return(__attribute__((unused)) void *rsp, __attribute__((unused)) void *arg)
{
    // @arch
    asm(
    "    movq %rdi, %rsp\n"
    "    movq %rsi, %rax\n"
    "    popq %r15\n"
    "    popq %r14\n"
    "    popq %r13\n"
    "    popq %r12\n"
    "    popq %rbx\n"
    "    popq %rbp\n"
    "    popq %rdi\n"
    "    ret\n");
}

void generator_switch_context(Generator *g, void *arg, void *rsp)
{
    g_generator_stack->rsp = rsp;
    g->parent = g_generator_stack;
    g_generator_stack = g;

    if (g->fresh) {
        g->fresh = false;
        // ******************************
        // ^                          ^rsp
        // stack_base
        void **rsp = (void**)((char*)g->stack_base + GENERATOR_STACK_CAPACITY);
        *(rsp-3) = arg;
        generator_restore_context(g->rsp);
    } else {
        generator_restore_context_with_return(g->rsp, arg);
    }
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
    "    jmp _generator_return\n"
#else
    "    jmp generator_return\n"
#endif
    );
}

void generator_return(void *arg, void *rsp)
{
    assert(g_generator_stack->parent != NULL);
    g_generator_stack->rsp = rsp;
    g_generator_stack = g_generator_stack->parent;
    generator_restore_context_with_return(g_generator_stack->rsp, arg);
}

void generator__finish_current(void)
{
    g_generator_stack->dead = true;
    g_generator_stack = g_generator_stack->parent;
    generator_restore_context_with_return(g_generator_stack->rsp, NULL);
}

Generator generator_create(void (*f)(void*))
{
    void* stack_base = mmap(NULL, GENERATOR_STACK_CAPACITY, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_STACK|MAP_ANONYMOUS|MAP_GROWSDOWN, -1, 0);
    assert(stack_base != MAP_FAILED);

    void **rsp = (void**)((char*)stack_base + GENERATOR_STACK_CAPACITY);
    *(--rsp) = generator__finish_current;
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
        .dead = false,
        .fresh = true,
        .parent = NULL,
    };
}

void generator_destroy(Generator *g)
{
    munmap(g->stack_base, GENERATOR_STACK_CAPACITY);
}

