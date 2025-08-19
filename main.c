#include "mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    void * base = malloc(1024);
    Arena arena = arena_init(base, 1024);
    uintptr_t * a = arena_push(&arena, 1);
    uintptr_t * b = arena_push(&arena, 100);
    uintptr_t * c = arena_push(&arena, 10);

    printf("a: %p, b: %p, c: %p\n", a, b, c);
}
