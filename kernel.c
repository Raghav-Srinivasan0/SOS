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

// Sleep
/*
void sleep(uint32_t millis)
{
	TimerIRQ(millis);
	while (millis > 0)
	{
		__asm__("sysSuspend:");
		__asm__ (" cli");
		__asm__ (" hlt");
		__asm__ (" jmp sysSuspend");
	}
}
*/
// CMOS Time
/*
void ReadFromCMOS (unsigned char array[])
{
	unsigned char tvalue, index;
	for(index = 0; index<128; index++)
	{
		cmos_read(index,tvalue);
		array[index] = tvalue;
	}
}

void WriteToCMOS(unsigned char array[])
{
	unsigned char index;
	for(index = 0; index<128; index++)
	{
		unsigned char tvalue = array[index];
		cmos_write(index,tvalue);
	}
}
*/
// SATA Disk

/*

// Detect attached SATA devices
#define	AHCI_BASE	0x400000	// 4M
 
#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000
 
void port_rebase(HBA_PORT *port, int portno)
{
	stop_cmd(port);	// Stop command engine
 
	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	port->clb = AHCI_BASE + (portno<<10);
	port->clbu = 0;
	memset((void*)(port->clb), 0, 1024);
 
	// FIS offset: 32K+256*portno
	// FIS entry size = 256 bytes per port
	port->fb = AHCI_BASE + (32<<10) + (portno<<8);
	port->fbu = 0;
	memset((void*)(port->fb), 0, 256);
 
	// Command table offset: 40K + 8K*portno
	// Command table size = 256*32 = 8K per port
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(port->clb);
	for (int i=0; i<32; i++)
	{
		cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
					// 256 bytes per command table, 64+16+48+16*8
		// Command table offset: 40K + 8K*portno + cmdheader_index*256
		cmdheader[i].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
		cmdheader[i].ctbau = 0;
		memset((void*)cmdheader[i].ctba, 0, 256);
	}
 
	start_cmd(port);	// Start command engine
}
 
// Start command engine
void start_cmd(HBA_PORT *port)
{
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR)
		;
 
	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST; 
}
 
// Stop command engine
void stop_cmd(HBA_PORT *port)
{
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;
 
	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;
 
	// Wait until FR (bit14), CR (bit15) are cleared
	while(1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
 
}

// Read hard disk sectors

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
 
bool read(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint16_t *buf)
{
	port->is = (uint32_t) -1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot(port);
	if (slot == -1)
		return false;
 
	HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;	// PRDT entries count
 
	HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) +
 		(cmdheader->prdtl-1)*sizeof(HBA_PRDT_ENTRY));
 
	// 8K bytes (16 sectors) per PRDT
	for (int i=0; i<cmdheader->prdtl-1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (uint32_t) buf;
		cmdtbl->prdt_entry[i].dbc = 8*1024-1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4*1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t) buf;
	cmdtbl->prdt_entry[i].dbc = (count<<9)-1;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;
 
	// Setup command
	FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;
 
	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		trace_ahci("Port is hung\n");
		return FALSE;
	}
 
	port->ci = 1<<slot;	// Issue command
 
	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1<<slot)) == 0) 
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			trace_ahci("Read disk error\n");
			return FALSE;
		}
	}
 
	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		trace_ahci("Read disk error\n");
		return FALSE;
	}
 
	return true;
}
 
// Find a free command list slot
int find_cmdslot(HBA_PORT *port)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i=0; i<cmdslots; i++)
	{
		if ((slots&1) == 0)
			return i;
		slots >>= 1;
	}
	trace_ahci("Cannot find free command list entry\n");
	return -1;
}

*/

// Sound

/*
static void play_sound(uint32_t nFrequence)
{
	uint32_t Div;
	uint8_t tmp;

	Div = 1193180 / nFrequence;
	ioport_out(0x43, 0xb6);
	ioport_out(0x42, (uint8_t)(Div));
	ioport_out(0x42, (uint8_t)(Div >> 8));

	//Play Sound using pc speakers
	tmp = ioport_in(0x61);
	if (tmp != (tmp | 3))
	{
		ioport_out(0x61, tmp | 3);
	}
}

static void nosound()
{
	uint8_t tmp = ioport_in(0x61) & 0xFC;

	ioport_out(0x61, tmp);
}

static void beep(uint32_t frequency, uint32_t millis)
{
	play_sound(frequency);
	sleep(millis);
	nosound();
}
*/
// IDT

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
