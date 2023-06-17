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

#include <stdint.h>
#include "keyboard_map.h"
//#include "disk.h"
#include <stddef.h>

extern void print_char_with_asm(char c, int row, int col);
extern void load_gdt();
extern void keyboard_handler();
extern char ioport_in(unsigned short port);
extern void ioport_out(unsigned short port, unsigned char data);
extern void load_idt(unsigned int* idt_address);
extern void enable_interrupts();
extern void cmos_read(unsigned char index, unsigned char tvalue);
extern void cmos_write(unsigned char index, unsigned char tvalue);
extern void TimerIRQ(uint32_t countdown);

char* terminal_buffer = (char*)0xb8000;
uint8_t buffer_position = 0;

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

struct IDT_entry IDT[IDT_SIZE];

//Keyboard Stuff

unsigned char inPortB (unsigned short _port)
{
	return ioport_in(_port);
}

void outPortB (unsigned short _port, unsigned char _data)
{
	ioport_out(_port,_data);
}

void init_idt()
{
	unsigned int offset = (unsigned int)keyboard_handler;
	IDT[0x21].offset_lowerbits = offset & 0x0000FFFF;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = IDT_INTERRUPT_GATE_32BIT;
	IDT[0x21].offset_upperbits = (offset & 0xFFFF0000) >> 16;

	outPortB(PIC1_COMMAND_PORT, 0x11);
	outPortB(PIC2_COMMAND_PORT, 0x11);

	outPortB(PIC1_DATA_PORT, 0x20);
	outPortB(PIC2_DATA_PORT, 0x20);

	outPortB(PIC1_DATA_PORT, 0x0);
	outPortB(PIC2_DATA_PORT, 0x0);

	outPortB(PIC1_DATA_PORT, 0x1);
	outPortB(PIC2_DATA_PORT, 0x1);

	outPortB(PIC1_DATA_PORT, 0xff);
	outPortB(PIC2_DATA_PORT, 0xff);

	struct IDT_pointer idt_ptr;
	idt_ptr.limit = (sizeof(struct IDT_entry) * IDT_SIZE) - 1;
	idt_ptr.base = (unsigned int) &IDT;
	load_idt(&idt_ptr);
}

void kb_init() 
{
	outPortB(PIC1_DATA_PORT, 0xFD);
}

/* Printchar implementation */

void printchar(char string, uint16_t c)
{
    terminal_buffer[buffer_position] = string;
    terminal_buffer[buffer_position+1] = (char)c;
    buffer_position+=2;
}


void handle_keyboard_interrupt()
{
	outPortB(PIC1_COMMAND_PORT, 0x20);
	unsigned char status = inPortB(KEYBOARD_STATUS_PORT);
	if (status & 0x1) 
	{
		unsigned char keycode = inPortB(KEYBOARD_DATA_PORT);
		if (keycode < 0 || keycode >= 128) return;
		if (keycode == 28)
		{
			buffer_position = (buffer_position+VGA_WIDTH*2)-(buffer_position%VGA_WIDTH*2);
		}
		printchar(keyboard_map[keycode], 0x0F);
	}
}


// VGA Graphics TODO: Fix problem of seeing gnu version in VGA. See myos.bin

uint16_t color(uint8_t foreground, uint8_t background)
{
	return (uint16_t)foreground | (uint16_t)background << 8;
}

void init_terminal(uint16_t c)
{
	uint16_t i;
	for(i = 0; i<VGA_WIDTH*VGA_HEIGHT; i++)
	{
		terminal_buffer[2*i] = ' ';
		terminal_buffer[2*i + 1] = (char)c;
	}
}

/* Printf implementation */

void printf(char* string, uint16_t c)
{
	size_t len = 0;
	while(string[len])
	{
		terminal_buffer[buffer_position] = string[len];
		terminal_buffer[buffer_position+1] = (char)c;
		buffer_position+=2;
		len++;
	}
}

char* scanf_result;

void scanf()
{
	scanf_result = "";
	uint8_t i;

	for(i = 0; i<(buffer_position%VGA_WIDTH); i++)
	{
		if(i%2==0)
			scanf_result[i/2] = terminal_buffer[buffer_position - buffer_position%VGA_WIDTH+i];
	}
}

void kernel_main(void)
{
	//beep(440,1000);
	init_terminal(color(10,2));
	printf("Hello World",color(11,8));
	init_idt();
	kb_init();
	enable_interrupts();
	while(1);
}
