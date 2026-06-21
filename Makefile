C_SOURCES = $(wildcard src/kernel/*.c src/drivers/*.c src/cpu/*.c)
HEADERS = $(wildcard src/include/*.h)
OBJ = ${C_SOURCES:.c=.o} src/boot/boot.o src/cpu/interrupt.o src/cpu/gdt_flush.o

CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -O2 -Wall -Wextra -Isrc/include -fno-stack-protector

all: kernel.bin

kernel.bin: src/linker.ld ${OBJ}
	ld -m elf_i386 -o $@ -T $< ${OBJ} --oformat binary

%.o: %.c ${HEADERS}
	${CC} ${CFLAGS} -c $< -o $@

%.o: %.s
	as --32 $< -o $@

clean:
	rm -rf *.bin *.dis *.o os-image.bin
	rm -rf src/kernel/*.o src/drivers/*.o src/boot/*.o src/cpu/*.o
