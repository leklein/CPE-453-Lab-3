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



void mt_sort(uint8_t* array) {
    uint8_t tid = get_thread_id();
    uint8_t low = 0;
    uint8_t high = 127;
    uint8_t key;
    uint8_t i;
    uint8_t j;
    
    mutex_lock(&mutex_screen);
    if (tid == 2) {
        low = 0;
        high = 31;
        set_cursor(21, 40);
    } else if (tid == 3) {
        low = 32;
        high = 63;
        set_cursor(26, 40);
    } else if (tid == 4) {
        low = 64;
        high = 95;
        set_cursor(31, 40);
    } else if (tid == 5) {
        low = 96;
        high = 127;
        set_cursor(36, 40);
    }
    // print out the unsorted partition, in red
    if (tid >= 2 && tid <= 5) {
        set_color(RED);
    }
    for (i = low; i <= high; i++) {
        print_int(arr[i]);
        print_string(" ");
    }
    mutex_unlock(&mutex_screen);
    // delay for readability
    _delay_ms(500);
    // wait
    sem_wait(&semaphore);
    _delay_ms(700);



    // sort the subsection
    for (i = low + 1; i <= high; i++) {
        key = arr[i];
        j = i - 1;

        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    // signal the semaphore since the thread has finished
    sem_signal(&semaphore);
    

    // print the sorted partitions
    mutex_lock(&mutex_screen);
    if (tid == 2) {
        low = 0;
        high = 31;       
        set_cursor(21, 40);   
    } else if (tid == 3) {
        low = 32;
        high = 63;
        set_cursor(26, 40);
    } else if (tid == 4) {
        low = 64;
        high = 95;
        set_cursor(31, 40);
    } else if (tid == 5) {
        low = 96;
        high = 127;
        set_cursor(36, 40);
    } else {
        low = 0;
        high = 127;
    }
    set_color(YELLOW);
    for (i = low; i <= high; i++) {
        print_int(arr[i]);
        print_string(" ");
    }

    
    mutex_unlock(&mutex_screen);
    _delay_ms(500);    
    

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
 
       set_cursor(2, 40);
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
        set_cursor(44, 0); // print array after the thread stats
        set_color(GREEN);
        uint8_t i;
        for (i = 0; i < 128; i++) {
            print_int(arr[i]);
            print_string(" ");
        }
        _delay_ms(500);

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

    sem_init(&semaphore, 4);

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

    
    

    
    create_thread("stat", (uint16_t) display_stats, (void*) "Sort", 200);
    create_thread("sort1", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("sort2", (uint16_t) mt_sort, (void*)arr, 200); 
    create_thread("sort3", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("sort4", (uint16_t) mt_sort, (void*)arr, 200);
    create_thread("print", (uint16_t) display_array, (void*) arr, 200);
    os_start();

    sei();

    // busy wait


    /*mutex_lock(&mutex_screen);
    //print_string("Final sort");
    mt_sort(arr);
    display_array(arr); 
    mutex_unlock(&mutex_screen);
    
    _delay_ms(1000);
    // re-init the sort
    for (i = 0; i < 128; i++) {
        arr[i] = rand() % 128;
    }  */

    while (1) {
        //busy wait
        while (semaphore.value < 4) {}
        mutex_lock(&mutex_screen);
        //print_string("Final sort");
        mt_sort(arr);
        display_array(arr); 
        mutex_unlock(&mutex_screen);
        
        _delay_ms(1000);
        // re-init the sort
        for (i = 0; i < 128; i++) {
            arr[i] = rand() % 128;
        }  
        

    }



    return 0;

}