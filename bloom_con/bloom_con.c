#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bloom_con.h"

#define XXH_STATIC_LINKING_ONLY   
#define XXH_IMPLEMENTATION        
#define XXH_INLINE_ALL
#include "xxhash.h"


// Configuration
#define BLOOMFILTER_BITS   (1U << 27)   /* 134 217 728 bits ≈ 16 MiB */


// handy helpers for bit/byte math 
#define BLOOM_BYTEIDX(bit)  ((bit) >> 3)
#define BLOOM_BITMASK(bit)  (1u << ((bit) & 7u))

#define HASH64(key,len)  XXH3_64bits((key), (len))

struct bloom_s {
    size_t                nbits;   // total number of bits
    _Atomic unsigned char *data;   // dynamic bit array                   
};


// Internal helpers (lock‑free)
static inline int bloom_get(const bloom_t *bf, uint32_t bit)
{
    unsigned char byte = atomic_load_explicit(&bf->data[BLOOM_BYTEIDX(bit)],memory_order_relaxed);
    return (byte & BLOOM_BITMASK(bit)) != 0;
}

static inline void bloom_set(bloom_t *bf, uint32_t bit)
{
    atomic_fetch_or_explicit(&bf->data[BLOOM_BYTEIDX(bit)],BLOOM_BITMASK(bit),memory_order_relaxed);
}


// Public API implementation
bloom_t *bloom_new(bloom_allocator alloc)
{
    if (!alloc) 
        alloc = malloc;
    bloom_t *bf = alloc(sizeof *bf);
    if (!bf){
        perror("bloom_new: allocation failed");
        return NULL;
    }
    bf->nbits = BLOOMFILTER_BITS;
    size_t nbytes = bf->nbits / 8;
    bf->data = (_Atomic unsigned char *)calloc(nbytes, 1);
    if (!bf->data) { 
        free(bf); 
        perror("bloom_new: data allocation failed");
        return NULL; 
    }
    return bf;
}

void bloom_destroy(bloom_t **pbf, bloom_deallocator dealloc)
{
    if (!pbf || !*pbf) 
        perror("bloom_destroy: NULL pointer\n");
        return;
    bloom_t *bf = *pbf;
    if (!dealloc) 
        dealloc = (bloom_deallocator)free; // default deallocator
    dealloc((void *)bf->data, 0);// free the bit array
    dealloc(bf, 0);// free the bloom filter itself
    *pbf = NULL; // clear dangling pointer
}

void bloom_clear(bloom_t *bf)
{
    memset((void *)bf->data, 0, bf->nbits / 8);
}

void bloom_add(bloom_t *bf, const void *key, size_t len)
{
    uint64_t h = HASH64(key, len);
    uint32_t bit1 = (uint32_t)(h >> 32) & (bf->nbits - 1);
    uint32_t bit2 = (uint32_t)h          & (bf->nbits - 1);
    bloom_set(bf, bit1);
    bloom_set(bf, bit2);
}

int bloom_test(bloom_t *bf, const void *key, size_t len)
{
    uint64_t h = HASH64(key, len);
    uint32_t bit1 = (uint32_t)(h >> 32) & (bf->nbits - 1);
    uint32_t bit2 = (uint32_t)h          & (bf->nbits - 1);
    return bloom_get(bf, bit1) && bloom_get(bf, bit2);
}


// Default allocators 

void *bloom_alloc(size_t sz){ 
    return malloc(sz); 
}
void  bloom_free(void *p, size_t _){ 
    (void)_; 
    free(p); 
}