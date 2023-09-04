.PHONY: all parts test clean disk

all: parts disk
parts:
	make -C boot all
	make -C kernel all
	make -C tools all
test: parts disk
	qemu-system-i386 -drive file=disk,format=raw -m 256M
clean:
	make -C boot clean
	make -C kernel clean
	make -C tools clean
	rm -f disk

disk:
	dd status=none if=/dev/zero of=disk bs=512 count=2048
	tools/fs-init disk
	tools/fs-copy disk boot/boot boot
	tools/fs-copy disk kernel/kernel kernel
