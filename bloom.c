#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#include "bloom.h"

#define XXH_STATIC_LINKING_ONLY /* access advanced declarations */
#define XXH_IMPLEMENTATION      /* access definitions */
#define XXH_INLINE_ALL
#include "xxhash.h"

// 2^27
#define BLOOMFILTER_SIZE 2^27
#define BLOOMFILTER_SIZE_BYTE BLOOMFILTER_SIZE / sizeof(volatile char)

#define hash(key, keylen) XXH3_64bits(key, keylen)

struct bloom_s {
    volatile char data[BLOOMFILTER_SIZE_BYTE];
};

static inline int get(bloom_t *filter, size_t key)
{
    uint64_t index = key / sizeof(char);
    uint8_t bit = 1u << (key % sizeof(char));
    return (filter->data[index] & bit) != 0;
}

static inline int set(bloom_t *filter, size_t key)
{
    uint64_t index = key / sizeof(char);
    uint64_t bit = 1u << (key % sizeof(char));
    return (atomic_fetch_or(&filter->data[index], bit) & bit) == 0;
}

bloom_t *bloom_new(bloom_allocator allocator)
{
    bloom_t *filter = allocator(sizeof(bloom_t));
    memset(filter, 0, sizeof(bloom_t));
    return filter;
}

void bloom_destroy(bloom_t **filter, bloom_deallocator deallocators)
{
    deallocators(*filter, sizeof(filter));
    *filter = NULL;
}

void bloom_clear(bloom_t *filter)
{
    memset(filter, 0, sizeof(bloom_t));
}

void bloom_add(bloom_t *filter, const void *key, size_t keylen)
{
    uint64_t hbase = hash(key, keylen);
    uint32_t h1 = (hbase >> 32) % BLOOMFILTER_SIZE;
    uint32_t h2 = hbase % BLOOMFILTER_SIZE;
    set(filter, h1);
    set(filter, h2);
}

int bloom_test(bloom_t *filter, const void *key, size_t keylen)
{
    uint64_t hbase = hash(key, keylen);
    uint32_t h1 = (hbase >> 32) % BLOOMFILTER_SIZE;
    uint32_t h2 = hbase % BLOOMFILTER_SIZE;
    return get(filter, h1) && get(filter, h2);
}

void *bloom_alloc(size_t size)
{
    return malloc(size);
}

void bloom_free(void *ptr, size_t size)
{
    (void) size; 
    (void) ptr;
    free(ptr);
}