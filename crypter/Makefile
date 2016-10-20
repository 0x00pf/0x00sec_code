all:prog crypter_rt polycrypt

crypter_rt:crypter_rt.c
	${CC} -Wall -o $@ $<

prog:prog.c
	${CC} -Wall -o $@ $<

polycrypt:polycrypt.c
	${CC} -Wall -o $@ $<
.PHONY:
clean:
	rm prog crypter_rt polycrypt
