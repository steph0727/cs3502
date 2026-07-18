#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdatomic.h>

#define NUM_ACCOUNTS 5
#define NUM_THREADS 4
#define TRANSACTIONS_PER_THREAD 20000
#define INITIAL_BALANCE 1000.00
#define MIN_AMOUNT 1
#define MAX_AMOUNT 100
#define VERBOSE_OPS 3

typedef struct{
 int account_id;
 double balance;
 int transaction_count;
}  Account;

Account accounts [NUM_ACCOUNTS];

atomic_llong expected_cents[NUM_ACCOUNTS];

typedef struct {
 int thread_id;
} ThreadData;

void *teller_thread(void *arg) {
 ThreadData *data = (ThreadData *)arg;
 int id = data->thread_id;
 unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self() ^ (unsigned int)id;

   for (int i = 0; i < TRANSACTIONS_PER_THREAD; i++) {
        int acc = rand_r(&seed) % NUM_ACCOUNTS;
        int is_deposit = rand_r(&seed) % 2;
        double amount = (double)(rand_r(&seed) % (MAX_AMOUNT - MIN_AMOUNT + 1) + MIN_AMOUNT);
        if (!is_deposit) amount = -amount;

	double current = accounts[acc].balance;
	struct timespec ts = {0, 2000};
	nanosleep(&ts, NULL);

	accounts[acc].balance = current + amount;
	accounts[acc].transaction_count++;
	atomic_fetch_add(&expected_cents[acc], (long long)llround(amount * 100.0));
	if (i < VERBOSE_OPS || i == TRANSACTIONS_PER_THREAD - 1) {
            printf("Thread %d: %-10s $%6.2f on Account %d (op #%d)\n", id, is_deposit ? "Depositing" : "Withdrawing", fabs(amount), acc, i);
        }
    }

  printf("Thread %d: finished %d transactions\n", id, TRANSACTIONS_PER_THREAD);
  return NULL;
}

int main(void) {
    printf("=== Phase 1 ===\n\n");

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        accounts[i].transaction_count = 0;
        atomic_init(&expected_cents[i], 0);
        printf("Account %d initial balance: %.2f\n", i, accounts[i].balance);
    }

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        int rc = pthread_create(&threads[i], NULL, teller_thread, &thread_data[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(rc));
            exit(1);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n=== Results: Expected vs Actual ===\n");
    double total_expected = 0.0, total_actual = 0.0;
    int mismatches = 0;

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        double expected = INITIAL_BALANCE + (double)atomic_load(&expected_cents[i]) / 100.0;
        double actual = accounts[i].balance;
        total_expected += expected;
        total_actual += actual;

        int mismatch = fabs(expected - actual) > 0.001;
        if (mismatch) mismatches++;

        printf("Account %d: expected = %10.2f  actual = %10.2f  %s\n",
               i, expected, actual, mismatch ? "<-- MISMATCH (lost update)" : "(match)");
    }

    printf("\nTotal expected: %.2f\n", total_expected);
    printf("Total actual:   %.2f\n", total_actual);
    return 0;
}
