#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "globals.h"
#include "os.h"
#include "synchro.h"

mutex_t mutex_screen;
extern system_t sysArray;

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

void blink(uint16_t *delay) {
   while(1) {
      led_off();
      thread_sleep((*delay)/10);
      led_on();
      thread_sleep((*delay)/10);
   }
}

void producer() {
   while(1) {
      //thread_sleep(100);
   }
}

void consumer() {
   while(1) {

   }
}

void display_stats(uint8_t *str) {
   thread_sleep(150);
   clear_screen();

   while(1) {
      mutex_lock(&mutex_screen);

      /*print_string("d ");
      set_color(YELLOW);
      set_cursor(12, 1);
      print_string(str);

      set_cursor(13, 1);
      set_color(32);
      print_string("Time: ");
      print_int(get_time());

      set_cursor(14, 1);
      set_color(36);
      print_string("Number of threads: ");
      print_int(get_num_threads());*/
      print_string("D");

      mutex_unlock(&mutex_screen);
      thread_sleep(10);
   }
}

void display_buffer() {
   while(1) {
      mutex_lock(&mutex_screen);

      //set_color(RED);
      //set_cursor(1, 1);
      print_string("B");

      mutex_unlock(&mutex_screen);
      thread_sleep(10);
   }
}

void idle() {
   while(1) {
      print_string("i");
   }
}

void main(void) {
   uint16_t blink_delay = 50;
   uint8_t string[15] = "Program 3";

   os_init();

   mutex_init(&mutex_screen);
   //record information about main thread

   //TODO change the "idle" thread to actually be in main, not idle()
   create_thread("main", (uint16_t)idle, (void*)NULL, 50);
   create_thread("producer", (uint16_t)producer, (void*)NULL, 50);
   create_thread("consumer", (uint16_t)consumer, (void*)NULL, 50);
   create_thread("stats", (uint16_t)display_stats, (void*)string, 50);
   create_thread("buffer", (uint16_t)display_buffer, (void*)NULL, 50);
   create_thread("blink", (uint16_t)blink, (void*)&blink_delay, 50);

   os_start();
   sei();
   while(1) {
      //TODO delete
      //print_string("M");
   }
}
