all:pwget nwget nwget1 fwget fwget1

nwget1: nwget1.c minios.s
	${CC} -static -nostartfiles -nostdlib -o $@ nwget1.c minios.s
	strip -s $@ -o $@.stripped

nwget: nwget.c
	diet ${CC} -Os -o nwget nwget.c
	strip -s nwget

pwget: pwget.asm
	nasm -f elf64 pwget.asm -o pwget.o
	ld -o pwget pwget.o
	chmod +x pwget

fwget: fwget.asm
	nasm -f bin -o fwget fwget.asm
	chmod +x fwget

fwget1: fwget1.asm
	nasm -f bin -o fwget1 fwget1.asm
	chmod +x fwget1

