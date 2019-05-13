all:elfpie_infect plpie

elfpie_infect:elfpie_infect.c
	${CC} -o $@ $<

plpie:plpie.asm
	nasm -f elf64 -o plpie.o plpie.asm
	ld -pie -o plpie plpie.o

.PHONY:
clean:
	rm elfpie_infect plpie.o plpie
