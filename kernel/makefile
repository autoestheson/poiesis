.PHONY: all clean

CROSS = ~/opt/cross/bin/i686-elf

all: kernel
clean:
	rm -f *.o
	rm -f kernel

kernel: entry.o kmain.o mon.o
	$(CROSS)-ld -T kernel.ld -o kernel entry.o kmain.o mon.o
entry.o: entry.asm
	nasm -f elf32 -o entry.o entry.asm
kmain.o: kmain.c
	$(CROSS)-gcc -I.. -ffreestanding -c -o kmain.o kmain.c
mon.o: mon.c
	$(CROSS)-gcc -I.. -ffreestanding -c -o mon.o mon.c
