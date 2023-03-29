export PATH="$HOME/opt/cross/bin:$PATH"

nasm -f elf32 kernel.asm -o boot.o

#i686-elf-as boot.s -o boot.o

# for the following command, if linking libraries, use: -Wl,-Bstatic -l<library name> 

gcc -m32 -ffreestanding -c kernel.c kernel.o
ld -m elf_i386 -T linker.ld -o myos.bin boot.o kernel.o
bash verify-bin.sh
bash make-iso.sh
