//Global defines
#ifndef GLOBALS_H
#define GLOBALS_H

#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37

void serial_init();
uint8_t byte_available();
uint8_t read_byte();
uint8_t write_byte(uint8_t b);
void print_string(uint8_t* s);
void print_int(uint16_t i);
void print_int32(uint32_t i);
void print_hex(uint16_t i);
void print_hex32(uint32_t i);
void set_cursor(uint8_t row, uint8_t col);
void set_color(uint8_t color);
void clear_screen(void);

#define THREAD_RUNNING 1
#define THREAD_READY 2
#define THREAD_SLEEPING 3
#define THREAD_WAITING 4

typedef struct thread_t {
   uint8_t thread_id;
   uint8_t thread_status;
   uint8_t sched_count;
   char name[10];
   uint16_t func;
   uint8_t *stack;
   uint8_t *stackPtr;
   uint8_t *stackBase;
   uint16_t size;
} thread_t;

typedef struct system_t {
   thread_t array[8];
   uint8_t threadsUsed;
   uint16_t sleep_counts[8];
   uint16_t sched_counts[8];
   uint8_t currThread;
   uint16_t num_interrupts;
} system_t;

#endif
