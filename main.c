#include "ccore.h"
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    printf("----REGULAR ARENA----\n");
    const size_t arr_len = 1024;
    const size_t size = 1 * MEGABYTE;
    void* base = malloc(size);
    Arena arena ={ 0 };
    arena_init(&arena, base, size);

    printf("%zu, %zu\n", sizeof(int), DEFAULT_ALIGNMENT);
    Allocator allocator = arena_allocator(&arena);
    int* arr = array(int, 16, &allocator);
    size_t old_used = 0;
    for (int i = 0; i < arr_len; i++) {
        array_append(arr, i);
        if (arena.used != old_used) {
            printf("Arena used: %zu/%zu\n", arena.used, arena.size);
            old_used = arena.used;
        }
    }

    printf("----VIRTUAL ARENA----\n");
    VArena varena;
    int result = varena_init(&varena, 1UL << 30);
    if (result != 0) {
        fprintf(stderr, "Error initializing varena.\n");
        return 1;
    }

    printf("%zu, %zu\n", sizeof(int), DEFAULT_ALIGNMENT);
    Allocator vallocator = varena_allocator(&varena);
    int* varr = array(int, 16, &vallocator);
    size_t old_offset = 0;
    for (int i = 0; i < arr_len; i++) {
        array_append(varr, i);
        if (varena.used != old_offset) {
            printf("VArena used: %zu/%zu\n", varena.used, varena.size);
            old_offset = varena.used;
        }
    }

    char* str = dynstr_init(8, &vallocator);
    for (int j = 0; j < 43; j++) {
        dynstr_append_c(str, '0' + j);
    }

    printf("%s\n", str);
}
