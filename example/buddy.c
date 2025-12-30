#include "../ccore.h"
#include <stdio.h>
#include <stdlib.h>

#define SIZE 8 * KILOBYTE

void
test_buddy_fragmentation_stress()
{
    printf("----BUDDY FRAGMENTATION STRESS----\n");
    void* data           = malloc(SIZE);
    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);
    Allocator alloc = buddy_allocator(&buddy);

    size_t small_size = 32;
    void* ptrs[SIZE / 32];
    int count = 0;

    while (count < (SIZE / small_size)) {
        ptrs[count] = alloc.alloc(small_size, &buddy);
        if (!ptrs[count])
            break;
        count++;
    }

    int i = 0;
    for (i = 0; i < count; i += 2) {
        alloc.free(ptrs[i], 0, &buddy);
    }

    void* large = alloc.alloc(SIZE / 2, &buddy);
    if (large == NULL) {
        printf("Success: Fragmentation handled (Large block refused as "
               "expected).\n");
    }

    free(data);
}
void
test_buddy_boundary_cases()
{
    printf("----BUDDY POWER-OF-TWO BOUNDARIES----\n");
    void* data           = malloc(SIZE);
    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);
    Allocator alloc = buddy_allocator(&buddy);

    void* p1 = alloc.alloc(2049, &buddy);
    void* p2 = alloc.alloc(2049, &buddy);

    void* p3 = alloc.alloc(1, &buddy);
    if (p3 == NULL) {
        printf("Correct: Boundary case waste managed.\n");
    }

    alloc.free(p1, 0, &buddy);
    alloc.free(p2, 0, &buddy);
    free(data);
}

void
test_buddy_realloc_exhaustion()
{
    printf("----BUDDY REALLOC EXHAUSTION----\n");
    void* data           = malloc(SIZE);
    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);
    Allocator alloc = buddy_allocator(&buddy);

    void* first   = alloc.alloc(SIZE / 4 - sizeof(BuddyBlock), &buddy);
    void* blocker = alloc.alloc(SIZE / 4 - sizeof(BuddyBlock), &buddy);

    void* grown = alloc.realloc(first,
                                SIZE / 4 - sizeof(BuddyBlock),
                                SIZE / 2 - sizeof(BuddyBlock),
                                &buddy);

    if (grown != first && grown != NULL) {
        printf("Success: Realloc moved data because buddy was blocked.\n");
    } else if (grown == NULL) {
        printf("Notice: Realloc failed (OOM), which is also valid behavior.\n");
    }

    free(data);
}

void
example_buddy_realloc_smaller()
{
    printf("----BUDDY REALLOC SMALLER----\n");
    void* data           = malloc(SIZE);
    BuddyAllocator buddy = { 0 };
    buddy_allocator_init(&buddy, data, SIZE, DEFAULT_ALIGNMENT);
    Allocator alloc  = buddy_allocator(&buddy);
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
    size_t arr_len  = 200;

    int* integers = array(int, 6, &alloc);
    int i         = 0;
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
    test_buddy_realloc_exhaustion();
    test_buddy_boundary_cases();
    test_buddy_fragmentation_stress();
    return 0;
}
