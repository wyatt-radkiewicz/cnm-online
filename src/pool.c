#include <stdlib.h>
#include "pool.h"
#include "console.h"
/*
#define NEXT_CHUNK_SIZE 1024

typedef struct _POOL_CHUNK POOL_CHUNK;
typedef union _ELEMENT_HEADER
{
	union _ELEMENT_HEADER *next;
	POOL_CHUNK *chunk;
} ELEMENT_HEADER;

typedef struct _POOL_CHUNK
{
	struct _POOL_CHUNK *next, *free;
	int size, allocs, index;
	POOL *pool;
	unsigned char *data;
	ELEMENT_HEADER *head;
} POOL_CHUNK;
typedef struct _POOL
{
	int biggest;
	int size;
	POOL_CHUNK *head, *free;
} POOL;

POOL *Pool_Create(int element_size)
{
	POOL *pool = malloc(sizeof(POOL));
	pool->biggest = NEXT_CHUNK_SIZE;
	pool->size = element_size + sizeof(ELEMENT_HEADER);
	pool->free = NULL;
	pool->head = NULL;
	return pool;
}
void Pool_Destroy(POOL *pool)
{
	POOL_CHUNK *chunk = pool->head, *next;
	while (chunk != NULL)
	{
		next = chunk->next;
		free(chunk->data);
		free(chunk);
		chunk = next;
	}
	free(pool);
}
void *Pool_Alloc(POOL *pool)
{
	POOL_CHUNK *chunk = pool->free;
	ELEMENT_HEADER *ptr;

	static int num_allocs = 0;
	num_allocs++;
	if (chunk == NULL)
	{
		chunk = malloc(sizeof(POOL_CHUNK));
		chunk->free = NULL;
		pool->free = chunk;
		chunk->next = pool->head;
		pool->head = chunk;
		chunk->size = pool->biggest;
		pool->biggest += NEXT_CHUNK_SIZE;
		chunk->allocs = 0;
		chunk->index = 0;
		chunk->pool = pool;
		chunk->data = malloc(pool->size * chunk->size);
		chunk->head = NULL;
	}

	if (chunk->index < chunk->size)
	{
		ptr = (ELEMENT_HEADER *)(chunk->data + pool->size * chunk->index++);
		ptr->chunk = chunk;
		if (++chunk->allocs == chunk->size)
			goto full;
		return ptr + 1;
	}
	else
	{
		ptr = chunk->head;
		ptr->chunk = chunk;
		chunk->head = ptr->next;
		if (++chunk->allocs == chunk->size)
			goto full;
		return ptr + 1;
	}

full:
	pool->free = pool->free->free;
	return ptr + 1;
}
void Pool_Free(POOL *pool, void *_ptr)
{
	ELEMENT_HEADER *ptr = (ELEMENT_HEADER *)_ptr - 1;
	POOL_CHUNK *chunk = ptr->chunk;
	int old_allocs = chunk->allocs;

	if (_ptr == NULL)
		return;
	ptr->next = chunk->head;
	chunk->head = ptr;
	chunk->allocs--;
	if (old_allocs == chunk->size && chunk->allocs != chunk->size)
	{
		chunk->free = pool->free;
		pool->free = chunk;
	}
}
*/