#include "ccore.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE 1 * MEGABYTE

typedef struct
{
    float x, y, z;
} Vec3;

int
main(void)
{
    void* base = malloc(SIZE);
    Pool pool  = { 0 };

    size_t chunk_size = 16 * KILOBYTE;
    pool_init(&pool, base, SIZE, chunk_size, DEFAULT_ALIGNMENT);
    Allocator alloc = pool_allocator(&pool);

    printf("--- Example: Stretchy Array ---\n");
    int* integers = make(int, 256, &alloc);
    int i         = 0;
    for (i = 0; i < 512; i++) {
        integers[i] = i;
    }
    printf("Integers[511]: %d\n", integers[511]);

    printf("\n--- Test 1: Chunk Reuse ---\n");
    Vec3* v1     = make(Vec3, 1, &alloc);
    void* v1_ptr = (void*)v1;
    printf("Allocated v1 at: %p\n", v1_ptr);

    pool_free(&pool, v1);

    Vec3* v2 = make(Vec3, 1, &alloc);
    printf("Allocated v2 at: %p\n", (void*)v2);

    if ((void*)v2 == v1_ptr) {
        printf("SUCCESS: Pool reused the freed chunk correctly.\n");
    }

    printf("\n--- Test 2: Alignment Check ---\n");
    for (i = 0; i < 5; i++) {
        void* p     = make(char, 1, &alloc);
        size_t addr = (size_t)p;
        printf("Addr: %p | Aligned: %s\n",
               p,
               (addr % DEFAULT_ALIGNMENT == 0) ? "YES" : "NO");
    }

    printf("\n--- Test 3: Pool Saturation ---\n");

    int total_chunks = SIZE / chunk_size;
    printf("Attempting to exhaust %d total chunks...\n", total_chunks);

    int count = 0;
    while (count < total_chunks + 1) {
        void* p = pool_allocate(&pool);
        if (!p) {
            printf("Pool successfully exhausted at chunk index: %d\n", count);
            break;
        }
        count++;
    }

    free(base);
    return 0;
}
