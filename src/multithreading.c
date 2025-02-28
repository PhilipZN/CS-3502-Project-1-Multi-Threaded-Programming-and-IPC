#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Bank account structure with an ID, balance, and associated mutex
typedef struct {
    int id;
    long balance;
    pthread_mutex_t mutex;
} Account;

#define NUM_ACCOUNTS 5  // number of accounts in simulation
Account accounts[NUM_ACCOUNTS];

// Parameter structure for deposit/withdraw operations
typedef struct {
    int thread_id;
    int acc_id;
    long amount;      // positive for deposit, negative for withdraw
    int use_mutex;    // flag to indicate if mutex locking should be used
} OpParams;

// Parameter structure for transfer operations
typedef struct {
    int thread_id;
    int from_acc;
    int to_acc;
    long amount;
    int use_deadlock_avoidance;  // flag for deadlock avoidance (Phase 4)
} TransferParams;

// Thread function for deposit or withdraw operation (Phase 1 & 2)
void* perform_operation(void* arg) {
    OpParams* p = (OpParams*) arg;
    int tid = p->thread_id;
    int acc = p->acc_id;
    long amount = p->amount;
    const char* opType = (amount >= 0) ? "Deposit" : "Withdraw";

    // Log thread start
    printf("[Thread %d] %s $%ld on Account %d (starting)\n", tid, opType, (amount >= 0 ? amount : -amount), acc);

    // Lock the account mutex if synchronization is enabled
    if (p->use_mutex) {
        pthread_mutex_lock(&accounts[acc].mutex);
        printf("[Thread %d] acquired lock on Account %d\n", tid, acc);
    }

    // Critical section: read-modify-write on account balance
    long old_balance = accounts[acc].balance;
    usleep(100000);  // simulate work (100ms) to increase chance of race condition
    long new_balance = old_balance + amount;
    accounts[acc].balance = new_balance;
    printf("[Thread %d] updated Account %d balance: %ld -> %ld\n", tid, acc, old_balance, new_balance);

    // Unlock mutex if it was locked
    if (p->use_mutex) {
        pthread_mutex_unlock(&accounts[acc].mutex);
        printf("[Thread %d] released lock on Account %d\n", tid, acc);
    }

    printf("[Thread %d] %s on Account %d (completed)\n", tid, opType, acc);
    pthread_exit(NULL);
}

// Thread function for transfer operation (Phase 3 & 4)
void* perform_transfer(void* arg) {
    TransferParams* p = (TransferParams*) arg;
    int tid = p->thread_id;
    int from = p->from_acc;
    int to   = p->to_acc;
    long amount = p->amount;
    printf("[Thread %d] Transfer $%ld from Account %d to Account %d (starting)\n", tid, amount, from, to);

    if (!p->use_deadlock_avoidance) {
        // Phase 3: Naive locking (prone to deadlock)
        pthread_mutex_lock(&accounts[from].mutex);
        printf("[Thread %d] locked Account %d, now trying to lock Account %d\n", tid, from, to);
        usleep(100000);  // delay to increase likelihood of deadlock
        pthread_mutex_lock(&accounts[to].mutex);
        printf("[Thread %d] locked Account %d\n", tid, to);

        // Perform transfer with both locks held
        accounts[from].balance -= amount;
        accounts[to].balance   += amount;
        printf("[Thread %d] transferred $%ld (Account %d new balance: %ld, Account %d new balance: %ld)\n",
               tid, amount, from, accounts[from].balance, to, accounts[to].balance);

        // Release locks
        pthread_mutex_unlock(&accounts[to].mutex);
        pthread_mutex_unlock(&accounts[from].mutex);
        printf("[Thread %d] Transfer completed and locks released\n", tid);
    } 
    else {
        // Phase 4: Deadlock avoidance using timeout and ordered locking
        int first = from;
        int second = to;
        // Enforce a consistent lock order (lower ID first) to prevent circular wait
        if (first > second) {
            first = to;
            second = from;
        }

        int got_first = 0, got_second = 0;
        const int max_retries = 5;
        int attempt = 0;
        while (attempt < max_retries) {
            attempt++;
            if (!got_first) {
                // Lock the first account
                pthread_mutex_lock(&accounts[first].mutex);
                got_first = 1;
                printf("[Thread %d] locked Account %d (first lock)\n", tid, first);
            }
            // Try to lock the second account without blocking indefinitely
            if (pthread_mutex_trylock(&accounts[second].mutex) == 0) {
                got_second = 1;
                printf("[Thread %d] locked Account %d (second lock)\n", tid, second);
            } else {
                // Could not acquire second lock – potential deadlock
                printf("[Thread %d] could not lock Account %d (held by another thread). Releasing Account %d and retrying...\n",
                       tid, second, first);
                // Release first lock to break deadlock and retry
                pthread_mutex_unlock(&accounts[first].mutex);
                got_first = 0;
                // Back-off before retrying to avoid livelock
                usleep(100000 + (rand() % 100000));  // 100–200ms random wait
                continue;  // try again
            }
            if (got_first && got_second) break;
        }

        if (got_first && got_second) {
            // Both locks acquired successfully
            accounts[from].balance -= amount;
            accounts[to].balance   += amount;
            printf("[Thread %d] transferred $%ld from Account %d to %d (new balances: %ld, %ld)\n",
                   tid, amount, from, to, accounts[from].balance, accounts[to].balance);
            // Release locks
            pthread_mutex_unlock(&accounts[second].mutex);
            pthread_mutex_unlock(&accounts[first].mutex);
            printf("[Thread %d] Transfer completed and locks released\n", tid);
        } else {
            // Failed to acquire both locks after retries – abort transfer to avoid deadlock
            if (got_first) pthread_mutex_unlock(&accounts[first].mutex);
            printf("[Thread %d] Transfer aborted to avoid deadlock\n", tid);
        }
    }

    printf("[Thread %d] Transfer from Account %d to %d (finished)\n", tid, from, to);
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));  // seed randomness for back-off timing
    printf("===== Project A: Multi-Threading Implementation =====\n");
    printf("Initializing %d accounts with $100 each.\n", NUM_ACCOUNTS);
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        accounts[i].id = i;
        accounts[i].balance = 100;
        pthread_mutex_init(&accounts[i].mutex, NULL);
    }

    /*** Phase 1: Basic Thread Operations ***/
    printf("\n---- Phase 1: Basic Thread Operations (No Mutex) ----\n");
    // Example: Two threads concurrently deposit and withdraw on Account 0 without locks
    accounts[0].balance = 100;
    printf("Account 0 initial balance: %ld\n", accounts[0].balance);
    OpParams p1 = {1, 0, 50, 0};   // Thread 1 will deposit $50 to Account 0
    OpParams p2 = {2, 0, -50, 0};  // Thread 2 will withdraw $50 from Account 0
    pthread_t t1, t2;
    pthread_create(&t1, NULL, perform_operation, &p1);
    pthread_create(&t2, NULL, perform_operation, &p2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Account 0 final balance: %ld (expected 100)\n", accounts[0].balance);
    if (accounts[0].balance != 100) {
        printf("** Race condition observed! Expected 100, got %ld **\n", accounts[0].balance);
    } else {
        printf("No race condition observed.\n");
    }

    /*** Phase 2: Resource Protection with Mutexes ***/
    printf("\n---- Phase 2: Resource Protection (Using Mutexes) ----\n");
    // Reset all accounts to $100 and perform concurrent operations with locks
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        accounts[i].balance = 100;
    }
    printf("All accounts reset to $100.\n");
    pthread_t threads[NUM_ACCOUNTS * 2];
    OpParams params[NUM_ACCOUNTS * 2];
    int tid_counter = 3;  // thread ID counter (continue from previous threads)
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        // Create one deposit and one withdraw thread for each account
        params[2*i]   = (OpParams){ tid_counter++, i, 50, 1 };   // deposit $50
        params[2*i+1] = (OpParams){ tid_counter++, i, -50, 1 };  // withdraw $50
        pthread_create(&threads[2*i],   NULL, perform_operation, &params[2*i]);
        pthread_create(&threads[2*i+1], NULL, perform_operation, &params[2*i+1]);
    }
    // Wait for all Phase 2 threads to finish
    for (int i = 0; i < NUM_ACCOUNTS * 2; ++i) {
        pthread_join(threads[i], NULL);
    }
    // Verify final balances for each account
    long total_balance = 0;
    for (int i = 0; i < NUM_ACCOUNTS; ++i) {
        total_balance += accounts[i].balance;
        printf("Account %d final balance: %ld (expected 100)\n", i, accounts[i].balance);
        if (accounts[i].balance != 100) {
            printf("** Account %d balance incorrect! **\n", i);
        }
    }
    printf("Total balance across all accounts: %ld (expected %ld)\n", total_balance, (long)NUM_ACCOUNTS * 100);
    if (total_balance != NUM_ACCOUNTS * 100) {
        printf("** Discrepancy in total balance detected! **\n");
    } else {
        printf("All account balances correct. Mutex synchronization successful.\n");
    }

    /*** Phase 3: Deadlock Creation ***/
    printf("\n---- Phase 3: Deadlock Creation ----\n");
    // Prepare two accounts and two threads that will deadlock
    accounts[0].balance = 100;
    accounts[1].balance = 100;
    printf("Account 0 balance = %ld, Account 1 balance = %ld\n", accounts[0].balance, accounts[1].balance);
    TransferParams tp1 = {1, 0, 1, 30, 0};  // Thread 1: transfer $30 from Account 0 -> 1 (no avoidance)
    TransferParams tp2 = {2, 1, 0, 20, 0};  // Thread 2: transfer $20 from Account 1 -> 0 (no avoidance)
    pthread_t td1, td2;
    pthread_create(&td1, NULL, perform_transfer, &tp1);
    pthread_create(&td2, NULL, perform_transfer, &tp2);
    sleep(1);  // allow time for deadlock to occur
    printf("Deadlock likely occurred (threads are waiting on each other).\n");
    printf("Proceeding to Phase 4 to resolve deadlock...\n");
    // **Note:** In a real deadlock, the program would hang here.
    // For demonstration, we cancel the deadlocked threads (they hold one lock each).
    pthread_cancel(td1);
    pthread_cancel(td2);
    pthread_mutex_destroy(&accounts[0].mutex);
    pthread_mutex_destroy(&accounts[1].mutex);
    pthread_mutex_init(&accounts[0].mutex, NULL);
    pthread_mutex_init(&accounts[1].mutex, NULL);
    // (We reinitialize the mutexes for accounts 0 and 1 to clear the deadlock state.)

    /*** Phase 4: Deadlock Resolution ***/
    printf("\n---- Phase 4: Deadlock Resolution ----\n");
    // Retry the same transfers with deadlock avoidance enabled
    accounts[0].balance = 100;
    accounts[1].balance = 100;
    printf("Account 0 balance = %ld, Account 1 balance = %ld\n", accounts[0].balance, accounts[1].balance);
    TransferParams tp3 = {3, 0, 1, 30, 1};  // Thread 3: transfer $30, with deadlock avoidance
    TransferParams tp4 = {4, 1, 0, 20, 1};  // Thread 4: transfer $20, with deadlock avoidance
    pthread_create(&td1, NULL, perform_transfer, &tp3);
    pthread_create(&td2, NULL, perform_transfer, &tp4);
    pthread_join(td1, NULL);
    pthread_join(td2, NULL);
    // Verify final balances and total
    printf("After transfers: Account 0 = %ld, Account 1 = %ld, Total = %ld (expected 200)\n",
           accounts[0].balance, accounts[1].balance, accounts[0].balance + accounts[1].balance);
    if (accounts[0].balance + accounts[1].balance != 200) {
        printf("** Total balance inconsistency detected! **\n");
    }
    printf("Multi-threading demonstration completed.\n");

    return 0;
}
