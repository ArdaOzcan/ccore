#include "../ccore.h"
#include <stdio.h>
#include <stdlib.h>

#define SIZE 8 * KILOBYTE

void
example_buddy_realloc_smaller()
{
    printf("----BUDDY REALLOC SMALLER----\n");
    void* data = malloc(SIZE);
    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);
    Allocator alloc = buddy_allocator(&buddy);
    int* large_array = make(int, 1000, &alloc);

    alloc.realloc(large_array, sizeof(int) * 1000, sizeof(int) * 200, &buddy);

    free(data);
}

void
example_buddy_realloc_larger()
{
    printf("----BUDDY REALLOC LARGER----\n");
    void* data = malloc(SIZE);

    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);

    Allocator alloc = buddy_allocator(&buddy);
    size_t arr_len = 200;

    int* integers = array(int, 6, &alloc);
    int i = 0;
    for (i = 0; i < arr_len; i++) {
        array_append(integers, i);
        printf("i: %d\n", i);
    }
    for (i = 0; i < arr_len; i++) {
        printf("%d, ", integers[i]);
    }
    printf("\n");

    free(data);
}

int
main(void)
{
    example_buddy_realloc_larger();
    example_buddy_realloc_smaller();
    return 0;
}
