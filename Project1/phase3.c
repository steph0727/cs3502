#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>

#define NUM_ACCOUNTS 2
#define INITIAL_BALANCE 1000.00
#define TRANSFER_AMOUNT 50.00
#define TRANSFER_ITERATIONS 10
#define WATCHDOG_CHECK_SECONDS 3
#define WATCHDOG_MAX_STALE_CHECKS 2

typedef struct {
    int account_id;
    double balance;
    pthread_mutex_t lock;
} Account;

Account accounts[NUM_ACCOUNTS];

atomic_int heartbeat = 0;

char thread_status[2][128] = {"starting", "starting"};
const char *thread_names[2] = {"Thread A", "Thread B"};

void transfer(int from_id, int to_id, double amount, int reporter_slot) {
    const char *name = thread_names[reporter_slot];
    snprintf(thread_status[reporter_slot], sizeof(thread_status[0]),
             "attempting transfer %d -> %d", from_id, to_id);
    printf("%s: Attempting transfer from %d to %d\n", name, from_id, to_id);
    atomic_fetch_add(&heartbeat, 1);

    pthread_mutex_lock(&accounts[from_id].lock);
    snprintf(thread_status[reporter_slot], sizeof(thread_status[0]),
             "HOLDING account %d, about to request account %d", from_id, to_id);
    printf("%s: Locked account %d\n", name, from_id);
    atomic_fetch_add(&heartbeat, 1);

usleep(200000);

 snprintf(thread_status[reporter_slot], sizeof(thread_status[0]),
             "HOLDING account %d, WAITING for account %d", from_id, to_id);
    printf("%s: Waiting for account %d\n", name, to_id);
    pthread_mutex_lock(&accounts[to_id].lock);

  atomic_fetch_add(&heartbeat, 1);
    accounts[from_id].balance -= amount;
    accounts[to_id].balance += amount;

    snprintf(thread_status[reporter_slot], sizeof(thread_status[0]), "completed transfer %d -> %d", from_id, to_id);
    printf("%s: Completed transfer %d -> %d\n", name, from_id, to_id);
    pthread_mutex_unlock(&accounts[to_id].lock);
    pthread_mutex_unlock(&accounts[from_id].lock);
}

void *thread_a(void *arg) {
    (void)arg;
    for (int i = 0; i < TRANSFER_ITERATIONS; i++) {
        transfer(0, 1, TRANSFER_AMOUNT, 0);
    }
    return NULL;
}

void *thread_b(void *arg) {
    (void)arg;
    for (int i = 0; i < TRANSFER_ITERATIONS; i++) {
        transfer(1, 0, TRANSFER_AMOUNT, 1);
    }
    return NULL;
}


void *watchdog(void *arg) {
    (void)arg;
    int stale_checks = 0;
    int last_seen = atomic_load(&heartbeat);

    while (1) {
        sleep(WATCHDOG_CHECK_SECONDS);
        int current = atomic_load(&heartbeat);

        if (current == last_seen) {
            stale_checks++;
            printf(" No progress for %d second(s)...\n",
                   stale_checks * WATCHDOG_CHECK_SECONDS);
        } else {
            stale_checks = 0;
        }
        last_seen = current;

        if (stale_checks >= WATCHDOG_MAX_STALE_CHECKS) {
            printf("\n*** DEADLOCK DETECTED ***\n");
            printf("No thread has made progress in %d seconds.\n",
                   stale_checks * WATCHDOG_CHECK_SECONDS);
            printf("Thread A status: %s\n", thread_status[0]);
            printf("Thread B status: %s\n", thread_status[1]);
            printf("Circular wait: Thread A holds Account 0 waiting on Account 1,\n");
            printf("               Thread B holds Account 1 waiting on Account 0.\n");
            printf("Terminating process.\n");
            fflush(stdout);
            exit(1);
        }
    }
    return NULL;
}

int main(void) {
    printf("=== Phase 3 ===\n\n");

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].account_id = i;
        accounts[i].balance = INITIAL_BALANCE;
        pthread_mutex_init(&accounts[i].lock, NULL);
        printf("Account %d initial balance: %.2f\n", i, accounts[i].balance);
    }
    printf("\nThread A will transfer(0 -> 1)\n");
    printf("Thread B will transfer(1 -> 0)\n\n");

    pthread_t ta, tb, wd;
    pthread_create(&wd, NULL, watchdog, NULL);
    pthread_create(&ta, NULL, thread_a, NULL);
    pthread_create(&tb, NULL, thread_b, NULL);

    pthread_join(ta, NULL);
    pthread_join(tb, NULL);

    printf("\nNo deadlock occurred this run (try again - timing is not guaranteed).\n");
    printf("Final balances: Account 0 = %.2f, Account 1 = %.2f\n",
           accounts[0].balance, accounts[1].balance);

    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }
    return 0;
}
