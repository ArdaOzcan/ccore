#include "ccore.h"
#include "vmem.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

size_t
system_page_size()
{
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

int
varena_destroy(VArena* varena)
{
    vmem_release(varena->base, varena->size);

    varena->base = NULL;
    varena->used = 0;
    varena->page_count = 0;
    varena->size = 0;
    return 0;
}

int
varena_init(VArena* arena, size_t size)
{
    return varena_init_ex(arena, size, system_page_size(), DEFAULT_ALIGNMENT);
}

int
varena_init_ex(VArena* arena, size_t size, size_t page_size, size_t alignment)
{
    assert(page_size % system_page_size() == 0);
    void* base = vmem_reserve(size);

#ifdef CCORE_VERBOSE
    printf("Reserved %zu bytes at %p\n", size, base);
#endif

    arena->base = base;
    arena->used = 0;
    arena->page_count = 0;
    arena->page_size = page_size;
    arena->size = size;
    arena->alignment = alignment;

    return 0;
}

static int
varena_commit_pages(VArena* varena, size_t amount)
{
    size_t committed = varena->page_count * varena->page_size;
    if (committed + varena->page_size * amount >= varena->size) {
        fprintf(stderr, "Allocation exceeds varena usable size!\n");
        return 1;
    }

    void* start = (uint8_t*)varena->base + committed;

    vmem_commit(start, varena->page_size * amount);

#ifdef CCORE_VERBOSE
    printf("Page committed at %p with size %zu.\n", start, varena->page_size);
#endif
    varena->page_count += amount;
    return 0;
}

static uintptr_t
align_forward(uintptr_t ptr, size_t alignment)
{
    assert((alignment & (alignment - 1)) == 0);
    uintptr_t modulo = ptr & (alignment - 1);
    if (modulo != 0) {
        ptr += alignment - modulo;
    }
    return ptr;
}

static void
varena_increase_capacity(VArena* varena, size_t size)
{
    size_t end_offset = varena->used + size;
    size_t committed = varena->page_size * varena->page_count;
    size_t bytes_needed = end_offset > committed ? end_offset - committed : 0;
    size_t pages_needed =
      (bytes_needed + varena->page_size - 1) / varena->page_size;

    varena->used += size;

    if (pages_needed == 0) {
        return;
    }

    int err = varena_commit_pages(varena, pages_needed);
    if (err != 0) {
        fprintf(stderr, "VArena: Error while committing pages.\n");
    }
}

void*
varena_push(VArena* varena, size_t size)
{
    size_t start_offset = align_forward(varena->used, varena->alignment);
    size_t end_offset = start_offset + size;
#ifdef CCORE_VERBOSE
    printf("Allocating %zu bytes in arena.\n", size);
    printf("Aligned from %zu to %zu.\n", varena->used, start_offset);
#endif

    varena_increase_capacity(varena, end_offset - varena->used);

    return (uint8_t*)varena->base + start_offset;
}

void
varena_push_copy(VArena* arena, const void* data, size_t size)
{
    memcpy(varena_push(arena, size), data, size);
}

void
arena_init_ex(Arena* arena, void* base, size_t size, size_t alignment)
{
    arena->base = base;
    arena->size = size;
    arena->alignment = alignment;
    arena->used = 0;
}

void
arena_init(Arena* arena, void* base, size_t size)
{
    arena_init_ex(arena, base, size, DEFAULT_ALIGNMENT);
}

static void*
arena_push_aligned(Arena* arena, size_t size, size_t alignment)
{
    arena->used = align_forward(arena->used, alignment);
    arena->used += size;
    if (arena->used > arena->size) {
        printf("Arena is full\n");
        return NULL;
    }

    return arena->base + arena->used - size;
}

void*
arena_push(Arena* a, size_t size)
{
    return arena_push_aligned(a, size, a->alignment);
}

void
arena_push_copy(Arena* arena, const void* data, size_t size)
{
    memcpy(arena_push(arena, size), data, size);
}

static void*
arena_alloc_(size_t bytes, void* context)
{
    return arena_push((Arena*)context, bytes);
}

static void*
arena_realloc_(void* start, size_t old_size, size_t new_size, void* context)
{
    return arena_push((Arena*)context, new_size);
}

static void
arena_free_(void* ptr, size_t bytes, void* context)
{
}

static void*
varena_alloc_(size_t bytes, void* context)
{
    return varena_push((VArena*)context, bytes);
}

static void
varena_free_(void* ptr, size_t bytes, void* context)
{
}

static void*
varena_realloc_(void* start, size_t old_size, size_t new_size, void* context)
{
    VArena* varena = context;
    // If at the end of the arena, we can just push
    // the required size and return the original pointer.
    if (start + old_size == varena->base + varena->used) {
        varena_increase_capacity(varena, new_size - old_size);
        return start;
    } else {
        return varena_push(varena, new_size);
    }
}

Allocator
arena_allocator(Arena* arena)
{
    return (Allocator){
        .alloc = arena_alloc_,
        .realloc = arena_realloc_,
        .free = arena_free_,
        .context = arena,
    };
}

Allocator
varena_allocator(VArena* varena)
{
    return (Allocator){
        .alloc = varena_alloc_,
        .realloc = varena_realloc_,
        .free = varena_free_,
        .context = varena,
    };
}

void*
array_init(size_t item_size, size_t capacity, Allocator* allocator)
{
    size_t size = item_size * capacity + sizeof(ArrayHeader);
    ArrayHeader* header = allocator->alloc(size, allocator->context);

    void* ptr = NULL;
    if (header) {
        header->capacity = capacity;
#ifdef CCORE_VERBOSE
        printf("Array initialized with capacity %zu\n", capacity);
#endif
        header->length = 0;
        header->item_size = item_size;
        header->allocator = allocator;
        ptr = header + 1;
    }

    return ptr;
}

void*
array_ensure_capacity(void* arr, size_t added_count)
{
    ArrayHeader* old_header = array_header(arr);

    size_t desired_capacity = old_header->length + added_count;
    if (desired_capacity <= old_header->capacity) {
        return arr;
    }

    // Realloc array
    size_t new_capacity = 2 * old_header->capacity;
    while (new_capacity < desired_capacity) {
        new_capacity *= 2;
    }

    size_t old_size =
      sizeof(ArrayHeader) + old_header->capacity * old_header->item_size;
    size_t new_size =
      sizeof(ArrayHeader) + new_capacity * old_header->item_size;
    ArrayHeader* new_header = old_header->allocator->realloc(
      old_header, old_size, new_size, old_header->allocator->context);

    if (new_header == NULL) {
        return NULL;
    }

#ifdef CCORE_VERBOSE
    printf(
      "Reallocing array from %zu bytes to %zu bytes.\n", old_size, new_size);
#endif

    if (new_header != old_header) {
#ifdef CCORE_VERBOSE
        printf("Copied memory.\n");
#endif
        memcpy(new_header, old_header, old_size);
    }

    if (old_header->allocator->free) {
        old_header->allocator->free(
          old_header, old_size, old_header->allocator->context);
    }

    new_header->capacity = new_capacity;
    return new_header + 1;
}

char*
cstr_from_dynstr(const char* src, Allocator* allocator)
{
    char* cstr = make(char, array_len(src) + 1, allocator);
    memcpy(cstr, src, array_len(src));
    cstr[array_len(src)] = '\0';
    return cstr;
}

char*
dynstr_from_cstr(const char* cstr, size_t capacity, Allocator* allocator)
{
    size_t len = strlen(cstr);

    char* arr = array(char, capacity, allocator);
    array_ensure_capacity(arr, len + 1);
    memcpy(arr, cstr, len);
    arr[len] = '\0';

    return arr;
}

char*
dynstr_init(size_t capacity, Allocator* a)
{
    char* arr = array(char, capacity, a);
    array_append(arr, '\0');

    return arr;
}

void
dynstr_append(char* dest, const char* src)
{
    size_t src_len = strlen(src);
    array_ensure_capacity(dest, src_len);
    size_t dest_str_len = dynstr_len(dest);
    array_header(dest)->length += src_len;
    memcpy(&dest[dest_str_len], src, src_len);
    dest[dynstr_len(dest)] = '\0';
}

void
dynstr_set(char* dest, const char* src)
{
    size_t src_len = dynstr_len(src);
    int diff = src_len - dynstr_len(dest);
    if (diff > 0) {
        array_ensure_capacity(dest, diff);
    }

    memcpy(dest, src, src_len);
    dest[src_len] = '\0';
    array_len(dest) = src_len + 1;
}

void
dynstr_shrink(char* str, size_t amount)
{
    // clamp
    amount = (amount > dynstr_len(str) ? dynstr_len(str) : amount);
    array_header(str)->length -= amount;
    str[array_len(str) - 1] = '\0';
}

void
dynstr_clear(char* str)
{
    array_len(str) = 1;
    str[0] = '\0';
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// From: https://benhoyt.com/writings/hash-table-in-c/
// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
uint64_t
hash_str(const char* key)
{
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

uint64_t
hash_strn(const char* key, size_t len)
{
    uint64_t hash = FNV_OFFSET;
    for (int i = 0; i < len; i++) {
        hash ^= (uint64_t)(unsigned char)(key[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}


void
hashmap_clear(Hashmap* hashmap)
{
    for (int i = 0; i < hashmap->capacity; i++) {
        hashmap->records[i].type = HASHMAP_RECORD_EMPTY;
        hashmap->records[i].key = NULL;
        hashmap->records[i].value = NULL;
    }
    hashmap->length = 0;
}

void
hashmap_init(Hashmap * hashmap, size_t capacity, Allocator* allocator)
{
    hashmap->records =
      allocator->alloc(sizeof(HashmapRecord) * capacity, allocator->context);
    hashmap->capacity = capacity;
    hashmap->length = 0;
    for (int i = 0; i < hashmap->capacity; i++) {
        hashmap->records[i].type = HASHMAP_RECORD_EMPTY;
        hashmap->records[i].key = NULL;
        hashmap->records[i].value = NULL;
    }
}

int
hashmap_insert(Hashmap* hashmap, char* key, void* value)
{
    if (value == NULL)
        return false;

    u16 idx = hash_str(key) % hashmap->capacity;
    for (int i = 0; i < hashmap->capacity; i++) {
        HashmapRecord* record =
          &hashmap->records[(idx + i) % hashmap->capacity];

        if (record->type == HASHMAP_RECORD_EMPTY ||
            record->type == HASHMAP_RECORD_DELETED) {
            record->key = key;
            record->value = value;
            record->type = HASHMAP_RECORD_FILLED;
            hashmap->length++;
            return 0;
        } else if (record->type == HASHMAP_RECORD_FILLED &&
                   strcmp(record->key, key) == 0) {
            return 1;
        }
    }

    return 1;
}

void*
hashmap_getn(Hashmap* hashmap, char* key, size_t key_len)
{
    u16 hash = hash_strn(key, key_len) % hashmap->capacity;
    for (int i = 0; i < hashmap->capacity; i++) {
        u16 idx = (hash + i) % hashmap->capacity;
        HashmapRecord* record = &hashmap->records[idx];
        if (record->type == HASHMAP_RECORD_EMPTY) {
            return NULL;
        }
        if (record->type == HASHMAP_RECORD_DELETED) {
            continue;
        }

        if (strcmp(key, record->key) == 0) {
            return record->value;
        }
    }

    return NULL;
}

void*
hashmap_get(Hashmap* hashmap, char* key)
{
    u16 hash = hash_str(key) % hashmap->capacity;
    for (int i = 0; i < hashmap->capacity; i++) {
        u16 idx = (hash + i) % hashmap->capacity;
        HashmapRecord* record = &hashmap->records[idx];
        if (record->type == HASHMAP_RECORD_EMPTY) {
            return NULL;
        }
        if (record->type == HASHMAP_RECORD_DELETED) {
            continue;
        }

        if (strcmp(key, record->key) == 0) {
            return record->value;
        }
    }

    return NULL;
}

void*
hashmap_delete(Hashmap* hashmap, char* key)
{
    u16 hash = hash_str(key) % hashmap->capacity;
    for (int i = 0; i < hashmap->capacity; i++) {
        u16 idx = (hash + i) % hashmap->capacity;
        HashmapRecord* record = &hashmap->records[idx];
        if (record->type == HASHMAP_RECORD_EMPTY) {
            return NULL;
        }
        if (record->type == HASHMAP_RECORD_DELETED) {
            continue;
        }

        if (strcmp(key, record->key) == 0) {
            void* temp = record->value;
            record->type = HASHMAP_RECORD_DELETED;
            record->key = NULL;
            record->value = NULL;
            return temp;
        }
    }

    return NULL;
}

void
hashmap_print(Hashmap* hashmap)
{
    printf("----START----\n");
    for (int i = 0; i < hashmap->capacity; i++) {
        HashmapRecord* record = &hashmap->records[i];
        if (record->type == HASHMAP_RECORD_FILLED)
            printf("(%d) %s: %s\n", i, record->key, (char*)record->value);
    }
    printf("----END----\n");
}
