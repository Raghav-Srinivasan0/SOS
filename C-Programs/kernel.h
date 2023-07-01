#ifndef VGA_WIDTH
#define VGA_WIDTH 80
#endif

#ifndef VGA_HEIGHT
#define VGA_HEIGHT 25
#endif

#define PIC1_COMMAND_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_COMMAND_PORT 0xA0
#define PIC2_DATA_PORT 0xA1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

#define IDT_SIZE 256
#define KERNEL_CODE_SEGMENT_OFFSET 0x8
#define IDT_INTERRUPT_GATE_32BIT 0x8e

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHTGRAY 7
#define DARKGRAY 8
#define LIGHTBLUE 9
#define LIGHTGREEN 10
#define LIGHTCYAN 11
#define LIGHTRED 12
#define LIGHTMAGENTA 13
#define YELLOW 13
#define WHITE 15

#include <stdint.h>
#include "keyboard_map.h"
//#include "disk.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include "multiboot.h"

void print(char* string);
void safe_println(char* string, int len);
void safe_print(char* string, int len);
void printchar(char c);
void printchar_at(char c, int row, int col);
extern void print_char_with_asm(char c, int row, int col);
void clear_screen();
void print_prompt();
void print_message();
void newline();
void backspace();

// IDT (from: https://www.youtube.com/watch?v=YtnNX074jMU&list=PL3Kz_hCNpKSTFCTJtP4-9mkYDVM7rAprW&index=12)

struct IDT_pointer {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

struct IDT_entry {
	unsigned short offset_lowerbits;
	unsigned short selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short offset_upperbits;
} __attribute__((packed));

extern void print_char_with_asm(char c, int row, int col);
extern void load_gdt();
extern void keyboard_handler();
extern char ioport_in(unsigned short port);
extern void ioport_out(unsigned short port, unsigned char data);
extern void load_idt(struct IDT_pointer* idt_address);
extern void enable_interrupts();
extern void cmos_read(unsigned char index, unsigned char tvalue);
extern void cmos_write(unsigned char index, unsigned char tvalue);
extern void TimerIRQ(uint32_t countdown);
extern void floppy_init();
extern void floppy_datarate(uint8_t datarate);
extern char floppy_dir();
extern void floppy_fifo(uint8_t command);

struct IDT_entry IDT[IDT_SIZE];

/* AHCI Stuff */

