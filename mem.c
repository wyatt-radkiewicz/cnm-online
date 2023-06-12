#include <stdlib.h>
#include "mem.h"
#include "xmacro_impl.h"

xmacro_as_str(_MEM_RESULT_XMACROS, mem_result_variants)

// Normal system malloc
static mem_result_t mem_system_alloc_interface(void *user_data, usize size) {
    if (!size) return (mem_result_t){ .val = nullptr, .result = MemResultAllocWithZeroSize };
    void *blk = malloc(size);
    if (blk != nullptr) {
        return (mem_result_t){ .val = blk, .result = MemResultOk };
    } else {
        return (mem_result_t){ .val = nullptr, .result = MemResultNonEnoughMem };
    }
}
static mem_result_t mem_system_realloc_interface(void *user_data, void *oldblk, usize newsz) {
    void *blk = oldblk;
    if (blk == nullptr && newsz) {
        blk = malloc(newsz);
    } else if (blk == nullptr && !newsz) {
        return (mem_result_t){ .val = nullptr, .result = MemResultAllocWithZeroSize };
    } else if (blk != nullptr && !newsz) {
        free(blk);
        blk = nullptr;
    } else {
        blk = realloc(blk, newsz);
    }
    return (mem_result_t){ .val = blk, .result = MemResultOk };
}
static mem_result_t mem_system_free_interface(void *user_data, void *blk) {
    free(blk);
    return (mem_result_t){ .val = nullptr, .result = (blk == nullptr ? MemResultFreedNullptr : MemResultOk) };
}
mem_allocator_interface_t mem_get_system_malloc_interface(void) {
    return (mem_allocator_interface_t) {
        .user_data = NULL,
        
    };
}

