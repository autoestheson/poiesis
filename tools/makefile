.PHONY: all clean

all: fs-init fs-copy
clean:
	rm -f fs-init fs-copy

fs-init:
	cc -I.. -o fs-init fs-init.c
fs-copy:
	cc -I.. -o fs-copy fs-copy.c
