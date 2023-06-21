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

char digitToChar(uint8_t digit)
{
	return (char)digit;
}

uint16_t color(uint8_t foreground, uint8_t background)
{
	return (uint16_t)foreground | (uint16_t)background << 8;
}

/* Printchar implementation */

/* Printf implementation */

// TODO: Fix the fact that only 130 characters can be printed to the screen before the cursor goes to the beginning

int cursor_row = 0;
int cursor_col = 0;
uint16_t currentColor = 0x0F;

void clear_screen() {
	for (int i = 0; i < VGA_HEIGHT; i++) {
		for (int j = 0; j < VGA_WIDTH; j++) {
			printchar_at(' ', i, j);
		}
	}
    cursor_row = 0;
}

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outPortB(0x3D4, 0x0A);
	outPortB(0x3D5, (inPortB(0x3D5) & 0xC0) | cursor_start);
 
	outPortB(0x3D4, 0x0B);
	outPortB(0x3D5, (inPortB(0x3D5) & 0xE0) | cursor_end);
}

void disable_cursor()
{
	outPortB(0x3D4, 0x0A);
	outPortB(0x3D5, 0x20);
}

void update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;
 
	outPortB(0x3D4, 0x0F);
	outPortB(0x3D5, (uint8_t) (pos & 0xFF));
	outPortB(0x3D4, 0x0E);
	outPortB(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

uint16_t get_cursor_position(void)
{
    uint16_t pos = 0;
    outPortB(0x3D4, 0x0F);
    pos |= inPortB(0x3D5);
    outPortB(0x3D4, 0x0E);
    pos |= ((uint16_t)inPortB(0x3D5)) << 8;
    return pos;
}

void printchar(char c) {
    printchar_at(c, cursor_row, cursor_col++);
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = cursor_col % VGA_WIDTH;
        cursor_row++;
    }
}

void printchar_at(char c, int row, int col) {
	// OFFSET = (ROW * 80) + COL
	char* offset = (char*) (0xb8000 + 2*((row * VGA_WIDTH) + col));
	*offset = c;
	offset[1] = (char)currentColor;
	update_cursor(col+1,row);
}

char* readline(int row)
{
	char* result = (char*) (0xb8000 + 2*((row * VGA_WIDTH)));
	return result;
}

void shiftUp()
{
	for (int i = 1; i<VGA_HEIGHT; i++)
	{
		char* line_data = readline(i);
		for (int j = 0; j<VGA_WIDTH*2; j+=2)
		{
			printchar_at(line_data[j], i-1,j/2);
		}
	}
	for (int j = 0; j<VGA_WIDTH*2; j+=2)
	{
		printchar_at(' ', VGA_HEIGHT-1,j/2);
	}
}

void newline() {
	if (cursor_row < VGA_HEIGHT-1)
	{
		cursor_row++;
	}
	else{
		shiftUp();
	}
    cursor_col = 0;
}

void println(char* line)
{
	int i = 0;
	while (line[i]!='\0')
	{
		printchar(line[i]);
		i++;
	}
	newline();
}

void print_init()
{
	println("                          ********   *******    ********              ");
	println("                         **//////   **/////**  **//////               ");
	println("                        /**        **     //**/**            Operating");
	println("       Srinivasan       /*********/**      /**/*********              ");
	println("                        ////////**/**      /**////////**              ");
	println("                               /**//**      **        /**    System   ");
	println("                         ********  //*******   ********               ");
	println("                        ////////    ///////   ////////                ");
}

void backspace() {
    if (cursor_col > 0) {
        print_char_with_asm(' ', cursor_row, --cursor_col);
    }
}

void shell(char* linedata)
{
	if (linedata[0] == 'e' && linedata[2] == 'c' && linedata[4] == 'h' && linedata[6] == 'o' && linedata[8] == ' ')
	{
		currentColor = color(MAGENTA,BLACK);
		for (int i = 10; i<VGA_WIDTH*2; i+=2)
		{
			printchar(linedata[i]);
		}
		for (int i = 0; i<4; i++)
		{
			printchar(' ');
		}
		newline();
		currentColor = color(WHITE,BLACK);
		return;
	}
	else if (linedata[0] == 'l' && linedata[2] == 'i' && linedata[4] == 'n' && linedata[6] == 'e' && linedata[8] == ' ')
	{
		char tensDigit = ' ';
		char onesDigit = ' ';
		if (linedata[12] != ' ')
		{
			onesDigit = linedata[12];
			tensDigit = linedata[10];
		}else{
			onesDigit = linedata[10];
			tensDigit = '0';
		}
		int number = ((int)tensDigit - 48) * 10 + ((int)onesDigit - 48);
		char* linedata = readline(number);
		currentColor = color(MAGENTA,BLACK);
		for (int i = 0; i<VGA_WIDTH*2; i+=2)
		{
			printchar(linedata[i]);
		}
		currentColor = color(WHITE,BLACK);
		return;
	}
}

void handle_keyboard_interrupt()
{
	outPortB(PIC1_COMMAND_PORT, 0x20);
	unsigned char status = inPortB(KEYBOARD_STATUS_PORT);
	if (status & 0x1) 
	{
		unsigned char keycode = inPortB(KEYBOARD_DATA_PORT);
		if (keycode < 0 || keycode >= 128) return;
		if (keycode == 14)
		{
			//printchar_at('b',10,10);
			backspace();
			return;
		}
		if (keycode == 28)
		{
			//printchar_at('e',10,10);
			char* line_data = readline(cursor_row);
			/* Implement Shell Commands Here */
			newline();
			shell(line_data);
			return;
		}
		if (keycode == 42 || keycode == 54)
		{
			//printchar_at('s',10,10);
			printchar((char)(((int)keyboard_map[keycode])-32));
		}else{
			printchar(keyboard_map[keycode]);
		}
	}
}

void kernel_main(void)
{
	enable_cursor(0,15);
	currentColor = color(LIGHTGREEN,CYAN);
	print_init();
	currentColor = color(WHITE,BLACK);
	init_idt();
	kb_init();
	enable_interrupts();
	while(1);
}
