#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdatomic.h>

#include "bloom.h"

#define TEST_DURATION 5      // seconds 
#define CONTAINS_P    85      // 85 % lookup, 15 % insert 
#define N_WORKERS     8


// Shared state                                                      
static bloom_t *g_filter;              
static pthread_mutex_t g_bloom_lock = PTHREAD_MUTEX_INITIALIZER; // coarse‑grained lock 
static _Atomic unsigned int g_ops = 0;    // total #operations
// static volatile int g_stop = 0;           
static _Atomic int g_stop = 0;            // flag to ask workers to exit

// Worker thread                                                     
static void *worker_loop(void *arg __attribute__((unused)))
{
    const u_int8_t key[] = "wiki.csie.ncku.edu.tw";
    u_int64_t key_len = strlen((const char *) key);
    int *k = (int *) key;
    srand(pthread_self());  // use thread ID as seed
    *k = rand();

    //while (!g_stop) 
    while (!atomic_load_explicit(&g_stop, memory_order_acquire)){
        int n = (*k) % 100;
        pthread_mutex_lock(&g_bloom_lock);
        if (n < CONTAINS_P)
            bloom_test(g_filter, key, key_len);
        else
            bloom_add(g_filter, key, key_len);
        pthread_mutex_unlock(&g_bloom_lock);

        (*k)++;
        atomic_fetch_add_explicit(&g_ops, 1, memory_order_relaxed);
    }
    return NULL;
}


int main(void)
{
    // init shared Bloom Filter
    g_filter = bloom_new(bloom_alloc);
    if (!g_filter) {
        perror("bloom_new");
        return 1;
    }

    // spawn worker threads 
    pthread_t tids[N_WORKERS];
    for (int i = 0; i < N_WORKERS; ++i) {
        if (pthread_create(&tids[i], NULL, worker_loop, NULL) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    sleep(TEST_DURATION);
    atomic_store_explicit(&g_stop, 1, memory_order_release);

    // join threads 
    for (int i = 0; i < N_WORKERS; ++i)
        pthread_join(tids[i], NULL);

    // 5. stats & cleanup 
    printf("%u ops/s", g_ops / TEST_DURATION);
    bloom_destroy(&g_filter, bloom_free);
    return 0;
}
