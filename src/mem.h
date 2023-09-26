#ifndef _MEM_H
#define _MEM_H
#include <stdbool.h>
#include "types.h"
#include "result.h"

#include "xmacro_start.h"
#define _MEM_RESULT_XMACROS \
    ENUM(MemResultOk) \
    ENUM(MemResultNonEnoughMem) \
    ENUM(MemResultBadAllocSize) \
    ENUM(MemResultSizeNotPowerOfTwo) \
    ENUM(MemResultInterfaceRequiresRealloc) \
    ENUM(MemResultInterfaceRequiresFree) \
    ENUM(MemResultAllocWithZeroSize) \
    ENUM(MemResultFreedNullptr)
xmacro_enum(_MEM_RESULT_XMACROS, mem_result_variants)
#include "xmacro_end.h"

make_result_type(mem, mem_result_variants_t, void *)

typedef mem_result_t (*mem_alloc_interface_t)(void *user_data, usize size);
typedef mem_result_t (*mem_realloc_interface_t)(void *user_data, void *blk, usize newsz);
typedef mem_result_t (*mem_free_interface_t)(void *user_data, void *blk);

typedef struct mem_allocator_interface_t {
    void *user_data;
    mem_alloc_interface_t alloc;
    mem_realloc_interface_t realloc;
    mem_free_interface_t free;
} mem_allocator_interface_t;

typedef struct mem_arena_t {
    mem_allocator_interface_t block_allocator;
    void *blocks;
    bool grows;
    bool initialized;
} mem_arena_t;

typedef struct mem_allocator_t {
    mem_allocator_interface_t block_allocator;
    void *blocks;
    bool grows;
    bool initialized;
    // other stuff
} mem_allocator_t;

typedef struct mem_pool_t {
    mem_allocator_interface_t block_allocator;
    void *blocks;
    bool grows;
    bool initialized;
    // other stuff
} mem_pool_t;

make_result_type(mem_arena_create, mem_result_variants_t, mem_arena_t)
make_result_type(mem_pool_create, mem_result_variants_t, mem_pool_t)
make_result_type(mem_allocator_create, mem_result_variants_t, mem_allocator_t)

mem_allocator_interface_t mem_get_system_interface(void);

mem_arena_create_result_t mem_arena_create(mem_allocator_interface_t block_allocator);
mem_arena_create_result_t mem_arena_create_with_initial_size(mem_allocator_interface_t block_allocator, usize initial_sz, bool can_grow);
mem_result_variants_t mem_arena_destroy(mem_arena_t *arena);
mem_result_t mem_arena_alloc(mem_arena_t *arena, usize size);
mem_allocator_interface_t mem_arena_interface(mem_arena_t *arena);

mem_pool_create_result_t mem_pool_create(mem_allocator_interface_t block_allocator, usize elem_size);
mem_pool_create_result_t mem_pool_create_with_initial_size(mem_allocator_interface_t block_allocator, usize elem_size, usize initial_sz, bool can_grow);
mem_result_variants_t mem_pool_destroy(mem_pool_t *pool);
mem_allocator_interface_t mem_pool_interface(mem_arena_t *arena);
mem_result_t mem_pool_alloc(mem_pool_t *pool);
mem_result_variants_t mem_pool_free(mem_pool_t *pool, void *blk);

mem_allocator_create_result_t mem_allocator_create(mem_allocator_interface_t block_allocator);
mem_allocator_create_result_t mem_allocator_with_initial_size(mem_allocator_interface_t block_allocator, usize initial_sz);
mem_result_variants_t mem_allocator_destroy(mem_allocator_t *allocator);
mem_allocator_interface_t mem_allocator_interface(mem_allocator_t *allocator);
mem_result_t mem_allocator_alloc(mem_allocator_t *allocator, usize size);
mem_result_t mem_allocator_realloc(mem_allocator_t *allocator, void *blk, usize newsize);
mem_result_t mem_allocator_free(mem_allocator_t *allocator, void *blk);

#endif
