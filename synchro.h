#ifndef SYNCHRO_H
#define SYNCHRO_H

typedef struct waitlist_buffer {
   uint8_t list[8];
   uint8_t head;
   uint8_t tail;
} waitlist_buffer;

typedef struct mutex_t {
   uint8_t owner_tid;
   uint8_t is_locked;
   waitlist_buffer waitlist;
} mutex_t;

typedef struct semaphore_t {
   uint8_t value;
   waitlist_buffer waitlist;
} semaphore_t;

void mutex_init(struct mutex_t* m);
void mutex_lock(struct mutex_t* m);
void mutex_unlock(struct mutex_t* m);
void sem_init(struct semaphore_t* s, int8_t value);
void sem_wait(struct semaphore_t* s);
void sem_signal(struct semaphore_t* s);
void sem_signal_swap(struct semaphore_t* s);

#endif