#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include "globals.h"
#include "os.h"
#include "synchro.h"

extern system_t sysArray;

void add_to_mutex_waitlist(struct mutex_t *m, uint8_t tid) {
   //push the tid to the tail, then increment
   m->waitlist.list[m->waitlist.tail] = tid;
   m->waitlist.tail = (m->waitlist.tail + 1) % 8;
}

uint8_t pop_from_mutex_waitlist(struct mutex_t *m) {
   uint8_t popped_tid;
   //if waitlist buffer is empty, return -1
   if (m->waitlist.head == m->waitlist.tail) return -1;
   //otherwise, pop the tid and increment the head of the list
   popped_tid = m->waitlist.list[m->waitlist.head];
   m->waitlist.head = (m->waitlist.head + 1) % 8;
   return popped_tid;
}

void add_to_sem_waitlist(struct semaphore_t *s, uint8_t tid) {
   //push the tid to the tail, then increment
   s->waitlist.list[s->waitlist.tail] = tid;
   s->waitlist.tail = (s->waitlist.tail + 1) % 8;
}

uint8_t pop_from_sem_waitlist(struct semaphore_t *s) {
   uint8_t popped_tid;
   //if waitlist buffer is empty, return -1
   if (s->waitlist.head == s->waitlist.tail) return -1;
   //otherwise, pop the tid and increment the head of the list
   popped_tid = s->waitlist.list[s->waitlist.head];
   s->waitlist.head = (s->waitlist.head + 1) % 8;
   return popped_tid;
}

void mutex_init(struct mutex_t* m) {
   //disable interrupts
   cli();
   //initialize mutex values
   m->is_locked = 0;
   m->owner_tid = -1;
   m->waitlist.head = 0;
   m->waitlist.tail = 0;
   //enable interrupts
   sei();
}

void mutex_lock(struct mutex_t* m) {
   uint8_t tid;
   //disable interrupts
   cli();
   //get current thread id
   tid = get_thread_id();
   //if mutex lock is in use by the same thread, do nothing
   //if it is in use by a different thread, add to waitlist
   if (m->is_locked) { 
      if (m->owner_tid != tid) {
         print_string("-mutex lock 1-");
         add_to_mutex_waitlist(m, tid);
         sysArray.array[tid].thread_status = THREAD_WAITING;
         yield();
      }
      else {
         print_string("-mutex lock 0-");
      }
      print_string("-mutex lock 3-");
   }
   //if mutex lock is not in use, give it to the thread
   else {
      print_string("-mutex lock 2-");
      m->is_locked = 1;
      m->owner_tid = tid;
   }
   //enable interrupts
   sei();
}

void mutex_unlock(struct mutex_t* m) {
   uint8_t tid, new_tid;
   //disable interrupts
   cli();
   //get current thread id
   tid = get_thread_id();
   //if mutex lock is unlocked, do nothing
   //if mutex lock is locked, check if it's owned by current thread
   if (m->is_locked) {
      //if mutex lock is owned by another thread, do nothing
      //if mutex lock is owned by current thread, check the waitlist
      if (m->owner_tid == tid) {
         new_tid = pop_from_mutex_waitlist(m);
         //if no other threads waiting
         if (new_tid == -1) {
            print_string("-mutex unlock 1-");
            m->is_locked = 0;
            m->owner_tid = -1;
         }
         //if there is another thread waiting
         else {
            print_string("-mutex unlock 2-");
            m->owner_tid = new_tid;
            sysArray.array[new_tid].thread_status = THREAD_READY;
         }
      }
   }
   //enable interrupts
   sei();
}

void sem_init(struct semaphore_t* s, int8_t value) {
   //disable interrupts
   cli();
   //intialize semaphore values
   s->value = value;
   s->waitlist.head = 0;
   s->waitlist.tail = 0;
   //enable interrupts
   sei();
}

void sem_wait(struct semaphore_t* s) {
   //disable interrupts
   cli();

   s->value--;
   if (s->value < 0) {
       uint8_t tid = get_thread_id();
       add_to_sem_waitlist(s, tid);
       sysArray.array[tid].thread_status = THREAD_WAITING;
       yield();
   } 
   //enable interrupts
   sei();
}

void sem_signal(struct semaphore_t* s) {
   //disable interrupts
   cli();

   s->value++;
   uint8_t new_tid = pop_from_sem_waitlist(s);
   if (new_tid != -1) {
      sysArray.array[new_tid].thread_status = THREAD_READY;
   }
   //enable interrupts
   sei();
}

void sem_signal_swap(struct semaphore_t* s) {
   //disable interrupts
   cli();
   s->value++;
   uint8_t new_tid = pop_from_sem_waitlist(s);
   uint8_t old_tid = get_thread_id();
   if (new_tid != -1) {
      sysArray.array[old_tid].thread_status = THREAD_READY;
      sysArray.array[new_tid].thread_status = THREAD_RUNNING;
      context_switch((uint16_t*)&sysArray.array[new_tid].stackPtr,
      (uint16_t*)&sysArray.array[old_tid].stackPtr);
   }
   //enable interrupts
   sei();
}