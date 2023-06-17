enter_protected:
	mov ah, 0x0
	mov al, 0x3
	int 0x10

	cli
	lgdt [gdt_descriptor]

	mov eax, cr0
	or eax, 0x1
	mov cr0, eax

	jmp CODE_SEG:pm_start
