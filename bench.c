#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "bloom.h"

#define TEST_DURATION 10
#define CONTAINS_P 85
#define N_WORKERS 8

typedef struct {
    int parent_fd;
    bloom_t *filter;
    uint32_t op_counter;
} globals_t;
globals_t globals;

void worker_loop()
{
    const u_int8_t key[] = "wiki.csie.ncku.edu.tw";
    u_int64_t key_len = strlen((const char *) key);
    int *k = (int *) key;
    srand(getpid());
    *k = rand();
    while (1) {
        int n = (*k) % 100;
        if (n < CONTAINS_P) {
            bloom_test(globals.filter, key, key_len);
        } else {
            bloom_add(globals.filter, key, key_len);
        }
        (*k)++;
        globals.op_counter++;
    }
}

void handle_sighup(int __attribute__((unused)) signal)
{
    /* Write total ammount off operations and exit */
    ssize_t wr =
        write(globals.parent_fd, &globals.op_counter, sizeof(uint32_t));
    if (wr == -1) {
        printf("%d:%s\n", errno, strerror(errno));
    }
    fflush(stdout);
    close(globals.parent_fd);
    exit(0);
}

typedef struct {
    int fd;
    pid_t pid;
} worker;

int create_worker(worker *wrk)
{
    int fd[2];

    int ret = pipe(fd);
    if (ret == -1)
        return -1;

    pid_t pid = fork();
    if (!pid) {
        /* Worker */
        close(fd[0]);
        globals.parent_fd = fd[1];
        worker_loop();
        exit(0);
    }
    if (pid < 0) {
        printf("ERROR[%d]:%s", errno, strerror(errno));
        close(fd[0]);
        close(fd[1]);
        return -1;
    }
    close(fd[1]);
    wrk->pid = pid;
    wrk->fd = fd[0];

    return 0;
}

int main(void)
{
    if (signal(SIGHUP, &handle_sighup) == SIG_ERR) {
        printf("ERROR[%d]:%s\n", errno, strerror(errno));
        exit(1);
    }

    worker workers[N_WORKERS];
    globals.filter = bloom_new(bloom_alloc);
    if (!globals.filter) {
        printf("ERROR: falied to make filter");
        exit(1);
    }

    /* create all workers */
    for (int i = 0; i < N_WORKERS; i++) {
        if (create_worker(&workers[i])) {
            bloom_destroy(&globals.filter, bloom_free);
            printf("ERROR[%d]:%s", errno, strerror(errno));
            exit(1);
        }
    }

    /* sleep a while */
    sleep(TEST_DURATION);

    /* Kill all workers */
    for (int i = 0; i < N_WORKERS; i++) {
        uint32_t worker_out = 0;
        if (kill(workers[i].pid, SIGHUP)) {
            bloom_destroy(&globals.filter, bloom_free);
            printf("ERROR[%d]:%s", errno, strerror(errno));
            exit(1);
        }
        (void) read(workers[i].fd, &worker_out, sizeof(uint32_t));
        globals.op_counter += worker_out;
    }
    bloom_destroy(&globals.filter, bloom_free);
    printf("%d ops/s\n", globals.op_counter / TEST_DURATION);
    return 0;
}