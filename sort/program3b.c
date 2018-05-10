#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include "globals.h"
#include "os.h"
#include "synchro.h"

uint8_t partition = 0;
uint8_t arr[128];
semaphore_t semaphore;   
extern system_t sysArray;

mutex_t mutex_screen;


void display_id_name_and_schedule(uint8_t row, thread_t* t) {
    set_cursor(row, 40);
    print_string("Thread ");
    print_int(t->thread_id);
    print_string(": ");
    print_string(t->name);
    print_string(" (scheduled ");
    print_int(t->sched_count);
    print_string(" times per second)     ");
 }

 void display_thread_status(uint8_t row, thread_t* t) {
    set_cursor(row, 40);
    print_string("Thread Status: ");
    if (t->thread_status == THREAD_RUNNING)
       print_string("THREAD_RUNNING     ");
    else if (t->thread_status == THREAD_READY)
       print_string("THREAD_READY     ");
    else if (t->thread_status == THREAD_SLEEPING)
       print_string("THREAD_SLEEPING     ");
    else if (t->thread_status == THREAD_WAITING)
       print_string("THREAD_WAITING     ");
 }

 void display_thread_pc(uint8_t row, thread_t* t) {
    set_cursor(row, 40);
    print_string("PC: ");
    print_hex((uint16_t)t->func);
 }

 void display_stack_info(uint8_t row, thread_t* t ) {
    set_cursor(row, 40);
    print_string("Stack base/end/size/usage: ");
    print_hex((uint16_t)t->stackBase);
    print_string("/");
    print_hex((uint16_t)t->stack);
    print_string("/");
    print_int(t->size);
    print_string("/");
    print_int((uint16_t)t->stackPtr - (uint16_t)t->stack);
 }

void display_thread_stats(uint8_t row_offset, thread_t *t) {
    display_id_name_and_schedule(++row_offset, t);
    display_thread_status(++row_offset, t);
    display_thread_pc(++row_offset, t);
    display_stack_info(++row_offset, t);
 }


void merge(uint8_t low, uint8_t mid, uint8_t high) {
    uint8_t l[mid - low + 1];
    uint8_t r[high - mid];
    uint8_t n1 = mid - low + 1;
    uint8_t n2 = n2 = high - mid;
    uint8_t i;
    uint8_t j;

    for (i = 0; i < n1; i++) {
        l[i] = arr[i + low];
    }

    for (i = 0; i < n2; i++) {
        r[i] = arr[i + mid + 1];
    }

    uint8_t k = low;
    i = j = 0;
    while (i < n1 && j < n2) {
        if (l[i] <= r[j])
            arr[k++] = l[i++];
        else
            arr[k++] = r[j++];
    }
 

    while (i < n1) {
        arr[k++] = l[i++];
    }

    while (j < n2) {
        arr[k++] = r[j++];
    }


}

void merge_t(uint8_t low, uint8_t high) {
    
    uint8_t mid = low + (high - low) / 2;
    
    if (low < high) {

        merge_t(low, mid);
        merge_t(mid + 1, high);
        merge(low, mid, high);
    }
}

void sort_t(uint8_t low, uint8_t high) {
    uint8_t p = partition++;
    //uint8_t low = p * (128 / 4);
    //uint8_t high = (p + 1) * (128 / 4) - 1;

    uint8_t mid = low + (high - low) / 2;

    if (high - low < 1) {
        return;
    }

    if (low < high) {
        merge_t(low, mid);
        merge_t(mid + 1, high);
        merge(low, mid, high);
    }   

    
}



void mt_sort(uint8_t* array) {

    uint8_t tid = get_thread_id();
    uint8_t low = 0;
    uint8_t high = 127;

 

    
    if (tid == 2) {
        low = 0;
        high = 31;
    } else if (tid == 3) {
        low = 32;
        high = 63;
    } else if (tid == 4) {
        low = 64;
        high = 95;
    } else if (tid == 5) {
        low = 96;
        high = 127;
    }

    sort_t(low, high);

    set_cursor(42, 0);
    print_string("current tid: ");
    print_int(tid);

    
    sem_wait(&semaphore);
    //sem_signal(&semaphore);
}


void display_stats(uint8_t *str) {
    uint8_t b;
    clear_screen();
 
    while(1) {
 
       mutex_lock(&mutex_screen);
 
       set_color(YELLOW);
       set_cursor(1, 40);
       print_string(str);
 
       set_cursor(2, 40);
       set_color(GREEN);
       print_string("Time: ");
       print_int(get_time());
 
       set_cursor(3, 40);
       set_color(CYAN);
       print_string("Number of threads: ");
       print_int(get_num_threads());
 
 
       set_color(WHITE);
       display_thread_stats(6, &sysArray.array[0]);
       display_thread_stats(11, &sysArray.array[1]);
       display_thread_stats(16, &sysArray.array[2]);
       display_thread_stats(21, &sysArray.array[3]);
       display_thread_stats(26, &sysArray.array[4]);
       display_thread_stats(31, &sysArray.array[5]);
 
       mutex_unlock(&mutex_screen);
       yield();
    }
 }

void display_array(uint8_t *arr) {
   
    while (1) {
        mutex_lock(&mutex_screen);
        set_cursor(80, 40); // print array after the thread stats
        set_color(YELLOW);
        uint8_t i;
        for (i = 0; i < 128; i++) {
            print_int(arr[i]);
            print_string(" ");
        }
        //sem_wait(&semaphore);

        mutex_unlock(&mutex_screen);
        yield();
    }

}



int main(char **argv) {

    os_init();

    // populate the array
    uint8_t i;
    for (i = 0; i < 128; i++) {

        arr[i] = rand() % 128;
    }    

    sem_init(&semaphore, 0);

    //sem_wait(&semaphore);

    sysArray.array[0].thread_id = 0;
    sysArray.array[0].thread_status = THREAD_RUNNING;
    sysArray.array[0].sched_count = 0;
    strncpy(sysArray.array[0].name, "main", 9);
    sysArray.array[0].func = (uint16_t)main;
    sysArray.array[0].stack = 0;
    sysArray.array[0].stackPtr = 0;
    sysArray.array[0].stackBase = 0;
    sysArray.array[0].size = 0;
 
    sysArray.threadsUsed++;
    clear_screen();

    
    

    
    create_thread("display_stats", (uint16_t) display_stats, (void*) "Multithreaded sort", 200);


    create_thread("sort1", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("sort2", (uint16_t) mt_sort, (void*)arr, 200); 
    create_thread("sort3", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("sort4", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("display_array", (uint16_t) display_array, (void*) arr, 200);


    os_start();



    return 0;

}