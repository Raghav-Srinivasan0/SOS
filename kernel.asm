bits 32
section .multiboot
	dd 0x1BADB002
	dd 0x0
	dd - (0x1BADB002 + 0x0)

section .text

%include "gdt.asm"

global _start
global load_gdt
global load_idt
global keyboard_handler
global ioport_in
global ioport_out
global enable_interrupts
global print_char_with_asm
;global read_from_disk
global cmos_read
global cmos_write
global TimerIRQ

extern kernel_main
extern handle_keyboard_interrupt

;read_from_disk:
;	pusha
;	push dx
TimerIRQ:
	push eax
	mov eax, [esp + 4]
	test eax, eax
	jz TimerDone
	dec eax
	mov [esp + 4], eax
TimerDone:
	pop eax
	iretd
cmos_read:
	cli
	mov al, [esp + 4]
	out 0x70,al
	in al,0x71
	sti
	mov [esp + 4 + 4],al
	ret
cmos_write:
	cli
	mov al, [esp + 4]
	out 0x70, al
	mov al, [esp + 4 + 4]
	out 0x71,al
	sti
	ret
print_char_with_asm:
	mov eax, [esp + 4 + 4]
	mov edx, 80
	mul edx
	add eax, [esp + 4 + 4 + 4]
	mov edx, 2
	mul edx
	mov edx, 0xb8000
	add edx, eax
	mov eax, [esp + 4]
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
