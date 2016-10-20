all:elf_infect payload

elf_infect:elf_infect.c
	${CC} -o $@ $<

payload:payload.asm
	nasm -f elf64 -o payload.o payload.asm
	ld -o payload payload.o

.PHONY:
clean:
	rm elf_infect payload.o payload
