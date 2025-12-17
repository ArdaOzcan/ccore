#include "ccore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 1 * MEGABYTE
int
main(void)
{
    void* base = malloc(SIZE);
    Pool pool = { 0 };
    pool_init(&pool, base, SIZE, 16 * KILOBYTE, DEFAULT_ALIGNMENT);

    Allocator alloc = pool_allocator(&pool);
    int * integers = array(int, 256, &alloc);
    int i = 0;
    for(i = 0; i < 512; i++) {
        array_append(integers, i);
    }

    char * string = make(char, 256, &alloc);
    strcpy(string, "Hello world!\n");

    for(i = 0; i < 512; i++) {
        printf("[%d]: %d\n", i, integers[i]);
    }
    printf("%s", string);

    return 0;
}
