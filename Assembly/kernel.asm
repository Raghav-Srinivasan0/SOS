bits 32
section .multiboot
	dd 0x1BADB002
	dd 0x0
	dd - (0x1BADB002 + 0x0)

section .text

%include "Assembly/gdt.asm"

global _start
global load_gdt
global load_idt
global keyboard_handler
global ioport_in
global ioport_out
global enable_interrupts
global print_char_with_asm

global floppy_init
global floppy_datarate
global floppy_dir
global floppy_fifo

extern kernel_main
extern handle_keyboard_interrupt

;read_from_disk:
;	pusha
;	push dx
;TimerIRQ:
;	push eax
;	mov eax, [esp + 4]
;	test eax, eax
;	jz TimerDone
;	dec eax
;	mov [esp + 4], eax
;TimerDone:
;	pop eax
;	iretd
;cmos_read:
;	cli
;	mov al, [esp + 4]
;	out 0x70,al
;	in al,0x71
;	sti
;	mov [esp + 4 + 4],al
;	ret
;cmos_write:
;	cli
;	mov al, [esp + 4]
;	out 0x70, al
;	mov al, [esp + 4 + 4]
;	out 0x71,al
;	sti
;	ret
print_char_with_asm:
	; OFFSET = (ROW * 80) + COL
	mov eax, [esp + 8] 		; eax = row
	mov edx, 80						; 80 (number of cols per row)
	mul edx								; now eax = row * 80
	add eax, [esp + 12] 	; now eax = row * 80 + col
	mov edx, 2						; * 2 because 2 bytes per char on screen
	mul edx
	mov edx, 0xb8000			; vid mem start in edx
	add edx, eax					; Add our calculated offset
	mov eax, [esp + 4] 		; char c
	mov [edx], al
	ret
load_gdt:
	lgdt [gdt_descriptor]
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	ret

enable_interrupts:
	sti
	ret

keyboard_handler:
	pushad
	cld
	call handle_keyboard_interrupt
	popad
	iret

ioport_in:
	mov edx, [esp + 4]
	in al, dx
	ret

ioport_out:
	mov edx, [esp + 4]
	mov eax, [esp + 8]
	out dx, al
	ret

floppy_init:
	mov dx, 0x3f2
	pusha
	mov al, 11111001b
	out dx, al
	popa
	ret

floppy_datarate:
	mov dx, 0x3f4
	pusha 
	mov eax, [esp + 4]
	out dx, al 
	popa 
	ret 

floppy_dir:
	mov dx, 0x3f7
	in al, dx 
	ret

floppy_fifo:
	mov dx, 0x3f5
	pusha
	mov eax, [esp + 4]
	out dx, al
	popa
	ret

_start:
	lgdt [gdt_descriptor]
	jmp CODE_SEG:.setcs
	.setcs:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, stack_space
	cli
	mov esp, stack_space
	call kernel_main
	hlt

section .bss
resb 8192
stack_space:
