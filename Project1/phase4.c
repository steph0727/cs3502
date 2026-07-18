#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define NUM_ACCOUNTS 5
#define NUM_THREADS 4
#define TRANSFERS_PER_THREAD 20000
#define INITIAL_BALANCE 1000.00
#define MIN_AMOUNT 1
#define MAX_AMOUNT 50
#define VERBOSE_OPS 3

typedef struct {
    int account_id;
    double balance;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

void safe_transfer(int from_id, int to_id, double amount) {
    if (from_id == to_id) return;

    int first  = (from_id < to_id) ? from_id : to_id;
    int second = (from_id < to_id) ? to_id   : from_id;

    pthread_mutex_lock(&accounts[first].lock);
    pthread_mutex_lock(&accounts[second].lock);

    accounts[from_id].balance -= amount;
    accounts[to_id].balance   += amount;

    pthread_mutex_unlock(&accounts[second].lock);
    pthread_mutex_unlock(&accounts[first].lock);
}

typedef struct {
    int thread_id;
} ThreadData;

void *transfer_thread(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int id = data->thread_id;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self() ^ (unsigned int)id;

    for (int i = 0; i < TRANSFERS_PER_THREAD; i++) {
        int from = rand_r(&seed) % NUM_ACCOUNTS;
        int to;
        do {
            to = rand_r(&seed) % NUM_ACCOUNTS;
        } while (to == from);
        double amount = (double)(rand_r(&seed) % (MAX_AMOUNT - MIN_AMOUNT + 1) + MIN_AMOUNT);

        safe_transfer(from, to, amount);

        if (i < VERBOSE_OPS || i == TRANSFERS_PER_THREAD - 1) {
            printf("Thread %d: transfer $%.2f  %d -> %d (op #%d)\n", id, amount, from, to, i);
        }
    }

    printf("Thread %d: finished %d transfers\n", id, TRANSFERS_PER_THREAD);
    return NULL;
}

void *thread_zero_to_one(void *arg) {
    (void)arg;
    for (int i = 0; i < 5000; i++) safe_transfer(0, 1, 10.0);
    return NULL;
}
void *thread_one_to_zero(void *arg) {
    (void)arg;
    for (int i = 0; i < 5000; i++) safe_transfer(1, 0, 10.0);
    return NULL;
}

int main(void) {
    printf("=== Phase 4 ===\n\n");

    double total_initial = 0.0;
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        pthread_mutex_init(&accounts[i].lock, NULL);
        total_initial += accounts[i].balance;
        printf("Account %d initial balance: %.2f\n", i, accounts[i].balance);
    }

    pthread_t p1, p2;
    struct timespec s1, e1;
    clock_gettime(CLOCK_MONOTONIC, &s1);
    pthread_create(&p1, NULL, thread_zero_to_one, NULL);
    pthread_create(&p2, NULL, thread_one_to_zero, NULL);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    clock_gettime(CLOCK_MONOTONIC, &e1);
    double replay_time = (e1.tv_sec - s1.tv_sec) + (e1.tv_nsec - s1.tv_nsec) / 1e9;
    printf("Replay completed in %.4f seconds with NO deadlock.\n\n", replay_time);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        pthread_create(&threads[i], NULL, transfer_thread, &thread_data[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n=== Results ===\n");
    double total_final = 0.0;
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("Account %d final balance: %.2f\n", i, accounts[i].balance);
        total_final += accounts[i].balance;
    }
    printf("\nTotal money before: %.2f\n", total_initial);
    printf("Total money after:  %.2f\n", total_final);
    printf("%s\n", (total_initial == total_final) ? "CONSERVED" : "MISMATCH (bug!)");
    printf("Elapsed time: %.4f seconds\n", elapsed);
    printf("No deadlocks occurred\n");

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
    return 0;
}
