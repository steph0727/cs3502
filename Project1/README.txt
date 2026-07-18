CS 3502 - Project 1: Multi-Threaded Banking System
====================================================

Build:
    gcc phase1.c -o phase1 -lm 
    gcc phase2.c -o phase2 -lm
    gcc phase3.c -o phase3 -lm
    gcc phase4.c -o phase4 -lm

Run:
    ./phase1
    ./phase2
    ./phase3 
    ./phase4

Approach per phase: see report.pdf for full write-up.

Summary:
Phase 1 demonstrates race conditions with unsynchronized reads/writes on
shared account balances. 

Phase 2 fixes this with a per-account pthread_mutex_t. 

Phase 3 deliberately deadlocks two threads transferring
in opposite lock order, detected by a watchdog thread that reports the
stall and terminates after ~6 seconds of no progress. 

Phase 4 resolves the deadlock using lock ordering.
