export PATH="$HOME/opt/cross/bin:$PATH"

nasm -i Assembly/ -f elf32 Assembly/kernel.asm -o Objects/boot.o
gcc -m32 -ffreestanding -c C-Programs/kernel.c -o Objects/kernel.o
ld -m elf_i386 -T Linker/linker.ld -o Objects/myos.bin Objects/boot.o Objects/kernel.o -z noexecstack
bash Shell-Scripts/verify-bin.sh
bash Shell-Scripts/make-iso.sh
