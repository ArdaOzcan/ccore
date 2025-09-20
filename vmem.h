#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

static void* vmem_reserve(size_t size) {
#ifdef _WIN32
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* ptr = mmap(NULL, size, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    return ptr == MAP_FAILED ? NULL : ptr;
#endif
}

static int vmem_commit(void* ptr, size_t size) {
#ifdef _WIN32
    return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != NULL;
#else
    return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

// static int vmem_decommit(void* ptr, size_t size) {
// #ifdef _WIN32
//     return VirtualFree(ptr, size, MEM_DECOMMIT) != 0;
// #else
//     return madvise(ptr, size, MADV_DONTNEED) == 0;
// #endif
// }

static void vmem_release(void* ptr, size_t size) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

