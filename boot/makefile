.PHONY: all clean

CROSS = ~/opt/cross/bin/i686-elf

all: bootblock bootblock.h boot
clean:
	rm -f *.o
	rm -f bootblock bootblock.h boot

bootblock.h: bootblock
	xxd -i bootblock > bootblock.h
bootblock: bootblock.asm
	nasm -f bin -o bootblock bootblock.asm

boot: boot1.o boot2.o mon.o
	$(CROSS)-ld -T boot.ld -o boot boot1.o boot2.o mon.o
boot1.o: boot1.asm
	nasm -f elf32 -o boot1.o boot1.asm
boot2.o: boot2.c
	$(CROSS)-gcc -I.. -ffreestanding -c -o boot2.o boot2.c
mon.o: mon.c
	$(CROSS)-gcc -I.. -ffreestanding -c -o mon.o mon.c
