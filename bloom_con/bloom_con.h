#pragma once

typedef struct bloom_s bloom_t;

typedef void *(*bloom_allocator)(size_t);
typedef void (*bloom_deallocator)(void *, size_t);

/* Contruction */
bloom_t *bloom_new(bloom_allocator allocators);
void bloom_destroy(bloom_t **filter, bloom_deallocator deallocators);

/* Operations */
void bloom_clear(bloom_t *filter);
void bloom_add(bloom_t *filter, const void *key, size_t keylen);
int bloom_test(bloom_t *filter, const void *key, size_t keylen);

/* Allocators */
void *bloom_alloc(size_t);
void bloom_free(void *filter, size_t);