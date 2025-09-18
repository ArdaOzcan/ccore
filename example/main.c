#include "ccore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
arena_example(void)
{
    printf("----REGULAR ARENA----\n");
    const size_t arr_len = 1024;
    const size_t size = 1 * MEGABYTE;
    void* base = malloc(size);
    Arena arena = { 0 };
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

    printf("----DYNAMIC STRING----\n");
    char* str = dynstr_init(8, &vallocator);
    for (int j = 0; j < 43; j++) {
        dynstr_append_c(str, '0' + j);
    }

    printf("%s\n", str);
    return 0;
}

int
main(void)
{
    ByteString bytes = byte_string_from_cstr("This is testing string!");
    printf("ByteString \"%.*s\" created with length %zu\n",
           (int)bytes.length,
           bytes.ptr,
           bytes.length);
    Hashmap hashmap = { 0 };
    VArena varena = { 0 };
    varena_init(&varena, 1 << 16);
    Allocator alloc = varena_allocator(&varena);
    hashmap_byte_string_init(&hashmap, 16, &alloc);
    printf("Hashmap initialized.\n");
    hashmap_print(&hashmap);

    int val = 1345;
    hashmap_insert(&hashmap, &bytes, &val);

    printf(
      "Inserted key-value pair %.*s : %d\n", (int)bytes.length, bytes.ptr, val);
    hashmap_print(&hashmap);

    int* lookup_val = (int*)hashmap_byte_string_get(
      &hashmap, byte_string_from_cstr("This is testing string!"));

    if(lookup_val == NULL) {
        fprintf(stderr, "Lookup value was not found\n");
        return 1;
    }

    printf("Lookup value was: %d\n", *lookup_val);

    return 0;
}
