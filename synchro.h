#ifndef SYNCHRO_H
#define SYNCHRO_H

typedef struct waitlist_node {
   uint16_t tid;
   struct waitlist_node *next;
} waitlist_node;

typedef struct mutex_t {
   uint8_t mid;
   uint8_t owner_tid;
   uint8_t is_locked;
   uint16_t *waitlist;
} mutex_t;

typedef struct semaphore_t {
   
} semaphore_t;

void mutex_init(struct mutex_t* m);
void mutex_lock(struct mutex_t* m);
void mutex_unlock(struct mutex_t* m);
void sem_init(struct semaphore_t* s, int8_t value);
void sem_wait(struct semaphore_t* s);
void sem_signal(struct semaphore_t* s);
void sem_signal_swap(struct semaphore_t* s);

#endif