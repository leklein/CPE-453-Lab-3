#include "globals.h"
#include "os.h"
#include "syncro.h"

waitlist_node* allocate_waitlist_node() {
   waitlist_node* n = (waitlist_node*) malloc(sizeof(waitlist_node));
   if (n == NULL) {
      fprintf(stderr, "PROGRAM 3: unable to allocate memory\n");
      exit(1);
   }
   return n;
}

void add_to_mutex_waitlist(struct mutex_t* m, uint16_t current_tid) {
   waitlist_node* current_waitlist_node = m->waitlist;

   waitlist_node* new_waitlist_node = allocate_waitlist_node();
   new_waitlist_node-tid = current_tid;
   new_waitlist_node->next = NULL;

   if (m->waitlist == NULL) m->waitlist = new_waitlist_node;
   else {
      while (current_waitlist_node->next != NULL) {
         current_waitlist_node = current_waitlist_node->next;
      }
      current_waitlist_node->next = new_waitlist_node;
   }
}

uint16_t pop_from_mutex_waitlist(struct mutex_t *m) {
   waitlist_node* popped_waitlist_node;
   uint16_t popped_tid;

   if (m->waitlist == NULL) return -1;

   popped_waitlist_node = m->waitlist;
   popped_tid = popped_waitlist_node->tid;
   m->waitlist = popped_waitlist_node->next;
   
   free(popped_waitlist_node);

   return popped_tid;
}

void add_to_sem_waitlist(struct semaphore_t* s, uint16_t current_tid) {
    waitlist_node* current_waitlist_node = s->waitlist;
 
    waitlist_node* new_waitlist_node = allocate_waitlist_node();
    new_waitlist_node-tid = current_tid;
    new_waitlist_node->next = NULL;
 
    if (s->waitlist == NULL) s->waitlist = new_waitlist_node;
    else {
       while (current_waitlist_node->next != NULL) {
          current_waitlist_node = current_waitlist_node->next;
       }
       current_waitlist_node->next = new_waitlist_node;
    }
 }
 
uint16_t pop_from_sem_waitlist(struct semaphore_t *s) {
    waitlist_node* popped_waitlist_node;
    uint16_t popped_tid;
 
    if (s->waitlist == NULL) return -1;
 
    popped_waitlist_node = s->waitlist;
    popped_tid = popped_waitlist_node->tid;
    s->waitlist = popped_waitlist_node->next;
    
    free(popped_waitlist_node);
 
    return popped_tid;
 }

void mutex_init(struct mutex_t* m) {
   //disable interrupts
   cli();
   //initialize mutex values
   m->is_locked = 0;
   m->owner_tid = -1;
   m->waitlist = NULL;
   //enable interrupts
   sei();
}

void mutex_lock(struct mutex_t* m) {
   uint16_t tid;
   //disable interrupts
   cli();
   //get current thread id
   tid = get_thread_id();
   //if mutex lock is in use by the same thread, do nothing
   //if it is in use by a different thread, add to waitlist
   if (m->is_locked) { 
      if (m->owner_tid != tid) {
         add_to_mutex_waitlist(m, tid);
         sysArray.array[tid].thread_status = THREAD_WAITING;
         yield();
      }
   }
   //if mutex lock is not in use, give it to the thread
   else {
      m->is_locked = 1;
      m->owner_tid = tid;
   }
   //enable interrupts
   sei();
}

void mutex_unlock(struct mutex_t* m) {
   uint16_t tid, new_tid;
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
            m->is_locked = 0;
            m->owner_tid = -1;
         }
         //if there is another thread waiting
         else {
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

   s->magnitude = value;
   s->remaining = value;
   s->waitlist = NULL;

   //enable interrupts
   sei();
}

void sem_wait(struct semaphore_t* s) {
   //disable interrupts
   cli();
   s->remaining--;
   if (s->remaining < 0) {
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

   s->remaining++;
   uint16_t new_tid = pop_from_sem_waitlist(s);
   if (new_tid != -1) {
      sysArray.array[new_tid].thread_status = THREAD_READY;
   }

   //enable interrupts
   sei();
}

void sem_signal_swap(struct semaphore_t* s) {
   //disable interrupts
   cli();
   s->remaining++;
   uint16_t new_tid = pop_from_sem_waitlist(s);
   uint_16_t old_tid = get_thread_id();
   if (new_tid != -1) {
      sysArray.array[old_tid].thread_status = THREAD_READY;
      sysArray.array[new_tid].thread_status = THREAD_RUNNING;
      context_switch((uint16_t*)&sysArray.array[new_tid].stackPtr,
      (uint16_t*)&sysArray.array[old_tid].stackPtr);

      
   }


   //enable interrupts
   sei();
}