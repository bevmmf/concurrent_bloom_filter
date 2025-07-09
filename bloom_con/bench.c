// bench.c — Throughput benchmark for the lock‑free (atomic) Bloom filter
// -----------------------------------------------------------------------------
// Build example:
//   gcc -O2 -std=c11 -pthread -o bench bench.c bloom.c
// Run:
//   ./bench            # prints <ops>/s aggregated across all threads
//
// The benchmark spawns N_WORKERS POSIX threads that continuously do
// 85 % membership tests and 15 % insertions for TEST_DURATION seconds.
// All threads share the same bloom_t instance that relies on atomic bit
// operations, so no additional locks are required in user space.
// -----------------------------------------------------------------------------

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bloom_con.h"

#define TEST_DURATION 10         
#define CONTAINS_P    85        
#define N_WORKERS      8

// Shared state                                                       
static bloom_t               *g_filter;      
static _Atomic uint64_t        g_op_counter; /* aggregated ops      */
static _Atomic int             g_stop = 0;   


static inline uint32_t xor_shift32(uint32_t *state)
{
    /* fast per‑thread PRNG (no locks) */
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static void *worker_thread(void *arg)
{
    (void)arg;
    uint32_t seed = (uint32_t)(uintptr_t)pthread_self();
    uint8_t  key[] = "wiki.csie.ncku.edu.tw";
    const size_t key_len = sizeof(key);

    while (!atomic_load_explicit(&g_stop, memory_order_relaxed)) {
        /* Quickly change the first 4 bytes to vary the key. */
        *(uint32_t *)key = xor_shift32(&seed);

        if ((seed % 100) < CONTAINS_P) {
            (void)bloom_test(g_filter, key, key_len);
        } else {
            bloom_add(g_filter, key, key_len);
        }
        atomic_fetch_add_explicit(&g_op_counter, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(void)
{
    g_filter = bloom_new(bloom_alloc);
    if (!g_filter) {
        fprintf(stderr, "ERROR: failed to create Bloom filter\n");
        return EXIT_FAILURE;
    }

    pthread_t tids[N_WORKERS];
    for (int i = 0; i < N_WORKERS; ++i) {
        if (pthread_create(&tids[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    sleep(TEST_DURATION);
    

    atomic_store_explicit(&g_stop, 1, memory_order_relaxed);
    for (int i = 0; i < N_WORKERS; ++i) {
        pthread_join(tids[i], NULL);
    }

    bloom_destroy(&g_filter, bloom_free);

    uint64_t total_ops = atomic_load_explicit(&g_op_counter, memory_order_relaxed);
    printf("%lu ops/s\n", total_ops / TEST_DURATION);
    return 0;
}
