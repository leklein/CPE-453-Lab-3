#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "os.h"
#include "synchro.h"

#define BUFFER_SIZE 10
#define BUFFER_TOP_ROW 2
#define BUFFER_COLUMN 18

extern system_t sysArray;

int8_t buffer_count = 0;

uint16_t production_rate = 100;
uint16_t consumption_rate = 100;

uint8_t production_animation_phase = 0;
uint8_t consumption_animation_phase = 0;

mutex_t mutex_screen;
mutex_t mutex_buffer;
mutex_t mutex_production_animation;
mutex_t mutex_consumption_animation;
semaphore_t sem_full;
semaphore_t sem_empty; 

void led_on() {
   asm volatile ("LDI R31, 0x00");
   asm volatile ("LDI R30, 0x24");
   asm volatile ("LD R25, Z"); // Load (Z) into R25.
   asm volatile ("SBR R25, 0x80"); // Set bit 7.
   asm volatile ("ST Z, R25"); // Store into Z location whatever is in R25.

   asm volatile ("LDI R31, 0x00");
   asm volatile ("LDI R30, 0x25");
   asm volatile ("LD R25, Z"); // Load (Z) into R25.
   asm volatile ("SBR R25, 0x80"); // Set bit 7.
   asm volatile ("ST Z, R25"); // Store into Z location whatever is in R25.
}

void led_off() {
   asm volatile ("LDI R31, 0x00");
   asm volatile ("LDI R30, 0x24");
   asm volatile ("LD R25, Z"); // Load (Z) into R25.
   asm volatile ("SBR R25, 0x80"); // Set bit 7.
   asm volatile ("ST Z, R25"); // Store into Z location whatever is in R25.

   asm volatile ("LDI R31, 0x00");
   asm volatile ("LDI R30, 0x25");
   asm volatile ("LD R25, Z"); // Load (Z) into R25.
   asm volatile ("CBR R25, 0x80"); // Clear bit 7.
   asm volatile ("ST Z, R25"); // Store into Z location whatever is in R25.
}

void blink() {
   while(1) {
      if (sem_empty.value < 0) {
         led_off();
      }
      else {
         led_on();
      }
   }
}

void producer() {
   while(1) {
      //produce an item
      thread_sleep(production_rate/2);
      production_animation_phase = 1;
      thread_sleep(production_rate/2);
      production_animation_phase = 2;
      //wait until there is space for the new item
      sem_wait(&sem_empty);
      mutex_lock(&mutex_production_animation);
      production_animation_phase = 0;
      mutex_unlock(&mutex_production_animation);
      //add new item to the buffer
      mutex_lock(&mutex_buffer);
      buffer_count++;
      mutex_unlock(&mutex_buffer);
      //indicate that there is another item in the buffer
      sem_signal(&sem_full);
   }
}

//TODO if consumer gets below 0 everything freezes up
void consumer() {
   uint8_t i;

   while(1) {
      //wait until there are items in the buffer
      sem_wait(&sem_full);
      //remove item from the buffer
      mutex_lock(&mutex_buffer);
      buffer_count--;
      mutex_unlock(&mutex_buffer);
      //indicate that there is one less item in the buffer
      sem_signal(&sem_empty);
      //consume an item
      thread_sleep(consumption_rate/5);
      thread_sleep(consumption_rate/5);
      mutex_lock(&mutex_consumption_animation);
      consumption_animation_phase = 2;
      mutex_unlock(&mutex_consumption_animation);
      thread_sleep(consumption_rate*3/5);
      mutex_lock(&mutex_consumption_animation);
      consumption_animation_phase = 0;
      mutex_unlock(&mutex_consumption_animation);
   }
}

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

void display_stats(uint8_t *str) {
   uint8_t b;
   clear_screen();

   while(1) {
      if (byte_available()) {
         b = read_byte();
         if (b == 'f' && production_rate < 1000) production_rate += 5;
         else if (b == 'r' && production_rate > 5) production_rate -= 5;
         else if (b == 'j' && consumption_rate < 1000) consumption_rate += 5;
         else if (b == 'u' && consumption_rate > 5) consumption_rate -= 5;
      }

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
      display_thread_stats(4, &sysArray.array[0]);
      display_thread_stats(9, &sysArray.array[1]);
      display_thread_stats(14, &sysArray.array[2]);
      display_thread_stats(19, &sysArray.array[3]);
      display_thread_stats(24, &sysArray.array[4]);
      display_thread_stats(29, &sysArray.array[5]);

      mutex_unlock(&mutex_screen);
      yield();
   }
}

void display_buffer_box(uint8_t row, uint8_t value) {
   set_cursor(BUFFER_TOP_ROW + row, BUFFER_COLUMN);
   print_string("| ");
   if (value == 0) print_string(" ");
   else print_string("O");
   print_string(" |");
}

void display_buffer_info() {
   set_cursor(13, 1);
   if (production_animation_phase != 0) set_color(YELLOW);
   else set_color(RED);
   print_string("Production Rate: ");
   print_int(production_rate*10);
   print_string(" ms     ");

   set_cursor(14, 1);
   if (consumption_animation_phase != 0) set_color(YELLOW);
   else set_color(RED);
   print_string("Consumption Rate: ");
   print_int(consumption_rate*10);
   print_string(" ms     ");

   set_color(RED);

   set_cursor(16, 1);
   print_string("Items In Buffer: ");
   if (sem_full.value > 0) print_int(sem_full.value);
   else print_int(0);
   print_string("       ");
}

void display_buffer_animations() {
   uint8_t i = 0;
   set_color(YELLOW);
   //draw production animation
   set_cursor(BUFFER_TOP_ROW, BUFFER_COLUMN - 5);
   for (i = 0; i < production_animation_phase; i++) {
      print_string("-");
   }
   print_string(">");
   print_string("  ");

   //draw consumption animation
   set_cursor(BUFFER_TOP_ROW, BUFFER_COLUMN + 7);
   for (i = 0; i < consumption_animation_phase; i++) {
      print_string("-");
   }
   print_string(">");
   print_string("  ");
}

void display_buffer() {
   uint8_t buffer_row;

   while(1) {
      mutex_lock(&mutex_screen);

      set_color(RED);

      mutex_lock(&mutex_buffer);

      for (buffer_row = 0; buffer_row < buffer_count; buffer_row++) {
         display_buffer_box(buffer_row, 1);
      }
      for (buffer_row = buffer_count; buffer_row < BUFFER_SIZE; buffer_row++) {
         display_buffer_box(buffer_row, 0);
      }

      mutex_unlock(&mutex_buffer);

      display_buffer_info();
      display_buffer_animations();

      mutex_unlock(&mutex_screen);
   }
}

void main(void) {
   uint8_t i;
   uint8_t string[15] = "Program 3";

   os_init();

   //initialize all mutexes/semaphores
   mutex_init(&mutex_screen);
   mutex_init(&mutex_buffer);
   mutex_init(&mutex_production_animation);
   mutex_init(&mutex_consumption_animation);
   sem_init(&sem_full, 0);
   sem_init(&sem_empty, BUFFER_SIZE);

   //record information about main thread
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

   //create other 5 threads
   create_thread("producer", (uint16_t)producer, (void*)NULL, 100);
   create_thread("stats", (uint16_t)display_stats, (void*)string, 50);
   create_thread("buffer", (uint16_t)display_buffer, (void*)NULL, 50);
   create_thread("consumer", (uint16_t)consumer, (void*)NULL, 100);
   create_thread("blink", (uint16_t)blink, (void*)NULL, 50);

   os_start();

   sei();

   //idle loop
   while(1) {}
}
