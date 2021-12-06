#
#  BareMetalOS Project Milestone 1
#  By: Geoffrey Tomlinson, Terry Cheney
#  For: Dr. Song
#  Created: 3/25/2021
#
#  Makefile to build everything needed to run our project
#

all: bootload kernel floppya.img loadFile shell
	./loadFile tstprg
	./loadFile tstpr2
	./loadFile message.txt
	./loadFile shell
	./loadFile phello 
	./loadFile bigmess.txt

bootload: bootload.asm
	nasm bootload.asm

kernel.o:kernel.c
	bcc -ansi -c -o kernel.o kernel.c

kernel_asm.o: kernel.asm
	as86 kernel.asm -o kernel_asm.o

kernel: kernel.o kernel_asm.o
	ld86 -o kernel -d kernel.o kernel_asm.o

floppya.img: kernel bootload
	dd if=/dev/zero of=floppya.img bs=512 count=2880
	dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc
	dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3
	dd if=message.txt of=floppya.img bs=512 count=1 seek=30 conv=notrunc
	dd if=map.img of=floppya.img bs=512 count=1 seek=1 conv=notrunc
	dd if=dir.img of=floppya.img bs=512 count=1 seek=2 conv=notrunc

shell.o: shell.c 
	bcc -ansi -c -o shell.o shell.c

lib.o: lib.asm
	as86 lib.asm -o lib.o
	
shell: shell.o lib.o
	ld86 -o shell -d shell.o lib.o

loadFile: loadFile.c
	gcc -o loadFile loadFile.c

clean: # removes all files made by this MakeFile
	rm -f *.o *.bin bootload floppya.img kernel loadFile

run: # runs the project on the qemu display
	export DISPLAY=:0.0
	qemu-system-i386 -fda floppya.img -device isa-debug-exit,iobase=0xf4,iosize=0x04 -boot order=a &