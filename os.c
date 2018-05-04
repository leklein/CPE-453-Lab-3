#include <avr/io.h>
#include <avr/interrupt.h>
#include "globals.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>

system_t sysArray;

uint16_t get_time() {
   return sysArray.num_interrupts / 100;
}

uint16_t get_thread_id() {
   return sysArray.currThread;
}

uint8_t get_next_thread() {
   //TODO modify so it picks only threads that are waiting, and updates sleep counters
   //TODO add sleep counters or some way to track sleeping threads
   //TODO maybe that should go in the ISR
   //TODO ask seng whether we need two ISRs
   return (sysArray.currThread+1) % sysArray.threadsUsed;
}

uint8_t get_num_threads() {
   return sysArray.threadsUsed;
}

__attribute__((naked)) void context_switch(uint16_t* new_tp, uint16_t* old_tp) {
   asm volatile("push R2");
   asm volatile("push R3");
   asm volatile("push R4");
   asm volatile("push R5");
   asm volatile("push R6");
   asm volatile("push R7");
   asm volatile("push R8");
   asm volatile("push R9");
   asm volatile("push R10");
   asm volatile("push R11");
   asm volatile("push R12");
   asm volatile("push R13");
   asm volatile("push R14");
   asm volatile("push R15");
   asm volatile("push R16");
   asm volatile("push R17");
   asm volatile("push R28");
   asm volatile("push R29");

   //--------
   asm volatile("ldi r31,0");
   asm volatile("ldi r30,0x5d");
   asm volatile("ld r17,Z+");
   asm volatile("ld r18,Z");

   asm volatile("mov r30,r22");
   asm volatile("mov r31,r23");
   asm volatile("st Z+,r17");
   asm volatile("st Z,r18");

   //--------
   
   asm volatile("mov r30,r24");
   asm volatile("mov r31,r25");
   asm volatile("ld r17,Z+");
   asm volatile("ld r18,Z");

   asm volatile("ldi r31,0");
   asm volatile("ldi r30,0x5d");
   asm volatile("st Z+,r17");
   asm volatile("st Z,r18");
   //--------
   
   asm volatile("pop R29");
   asm volatile("pop R28");
   asm volatile("pop R17");
   asm volatile("pop R16");
   asm volatile("pop R15");
   asm volatile("pop R14");
   asm volatile("pop R13");
   asm volatile("pop R12");
   asm volatile("pop R11");
   asm volatile("pop R10");
   asm volatile("pop R9");
   asm volatile("pop R8");
   asm volatile("pop R7");
   asm volatile("pop R6");
   asm volatile("pop R5");
   asm volatile("pop R4");
   asm volatile("pop R3");
   asm volatile("pop R2");
   asm volatile("ret"); //return to ISR
}

void yield() {
   cli();

   uint8_t old_tid = get_thread_id();
   //if current thread is not waiting or sleeping, set status to THREAD_READY
   if(sysArray.array[old_tid].thread_status == THREAD_RUNNING) {
      sysArray.array[old_tid].thread_status = THREAD_READY;
   }

   sysArray.currThread = get_next_thread();

   context_switch((uint16_t*)&sysArray.array[sysArray.currThread].stackPtr,
      (uint16_t*)&sysArray.array[old_tid].stackPtr);

   sei();
}

//This interrupt routine is automatically run every 10 milliseconds
ISR(TIMER0_COMPA_vect) {
   //The following statement tells GCC that it can use registers r18-r31,
   //for this interrupt routine.  These registers (along with r0 and r1) 
   //will automatically be pushed and popped by this interrupt routine.
   asm volatile ("" : : : "r18", "r19", "r20", "r21", "r22", "r23", "r24", \
                 "r25", "r26", "r27", "r30", "r31");

   sysArray.num_interrupts++;

   uint8_t old_tid = get_thread_id();
   sysArray.currThread = get_next_thread();

   //Call context switch here to switch to that next thread
   context_switch((uint16_t*)&sysArray.array[sysArray.currThread].stackPtr,
      (uint16_t*)&sysArray.array[old_tid].stackPtr);
}

//Call this to start the system timer interrupt
void start_system_timer() {
   TIMSK0 |= _BV(OCIE0A);  //interrupt on compare match
   TCCR0A |= _BV(WGM01);   //clear timer on compare match

   //Generate timer interrupt every ~10 milliseconds
   TCCR0B |= _BV(CS02) | _BV(CS00) | _BV(CS02);     //prescalar /1024
   OCR0A = 156;             //generate interrupt every 9.98 milliseconds
}

void os_init() {
   uint8_t i;
   sysArray.threadsUsed = 0;
   sysArray.currThread = 0;
   sysArray.num_interrupts = 0;
   thread_t init;
   init.func = 0;
   init.stack = NULL;
   init.stackPtr = NULL;
   for(i=0;i<8;i++) {
      sysArray.array[i] = init;
   }

   serial_init();
}

void os_start() {
   // start system timer
   start_system_timer();
   sei();

   //context switch
   uint16_t temp;
   context_switch((uint16_t*)&sysArray.array[0].stackPtr,&temp);
}

__attribute__((naked)) void thread_start(void) {
   sei(); //enable interrupts - leave this as the first statement in thread_start()
   // Load in arguments from R15:R14 to R25:R24.
   asm volatile("movw R24,R14");
   asm volatile("movw R30,R16"); // Put function pointer in Z.
   asm volatile("ijmp");
}

void create_thread(char* name, uint16_t address, void* args, uint16_t stack_size) {
   sysArray.array[sysArray.threadsUsed].thread_id = sysArray.threadsUsed;
   strncpy(sysArray.array[sysArray.threadsUsed].name,name,9);
   sysArray.array[sysArray.threadsUsed].thread_status = THREAD_READY;
   sysArray.array[sysArray.threadsUsed].sched_count = 0;

   sysArray.array[sysArray.threadsUsed].func = address;

   // Malloc space for the stack.
   sysArray.array[sysArray.threadsUsed].size = sizeof(regs_context_switch) +
      sizeof(regs_interrupt) + stack_size;
   sysArray.array[sysArray.threadsUsed].stack = 
      malloc(sysArray.array[sysArray.threadsUsed].size);

   // Move stack pointer to the top.
   sysArray.array[sysArray.threadsUsed].stackPtr = 
      sysArray.array[sysArray.threadsUsed].stack +
      sizeof(regs_context_switch) + sizeof(regs_interrupt) + stack_size - 1;

   // Record high address of stack.
   sysArray.array[sysArray.threadsUsed].stackBase =
      sysArray.array[sysArray.threadsUsed].stackPtr;

   // Move stack pointer to where it needs to be to pop the registers.
   sysArray.array[sysArray.threadsUsed].stackPtr -= sizeof(regs_context_switch);

   // Prepare the stack for context_switch.
   regs_context_switch *ptr = 
      (void*)sysArray.array[sysArray.threadsUsed].stackPtr;
   ptr->pcl = (uint16_t)thread_start&0xFF;
   ptr->pch = (uint16_t)thread_start>>8;
   ptr->eind= (uint16_t)0;

   ptr->r16 = address&0xFF;
   ptr->r17 = address>>8;
   ptr->r14 = (uint16_t)args&0xFF;
   ptr->r15 = (uint16_t)args>>8;

   // Count new thread in threadsUsed count.
   sysArray.threadsUsed++;
}
