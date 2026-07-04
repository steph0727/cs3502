
#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE 10
#define SHM_KEY 0x1234

#define SEM_MUTEX_NAME  "/sem_mutex"
#define SEM_EMPTY_NAME  "/sem_empty"
#define SEM_FULL_NAME   "/sem_full"

typedef struct {
int value; // The data
int producer_id; // Which producer created this item
} item_t;


typedef struct {
item_t buffer[BUFFER_SIZE];
int head; // Next write position (producer)
int tail; // Next read position (consumer)
int count; // Current number of items
} shared_buffer_t;

#endif
