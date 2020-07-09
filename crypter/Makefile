all:prog crypter_rt polycrypt

crypter_rt:crypter_rt.c
	${CC} ${CFLAGS} -Wall -o $@ $<

prog:prog.c
	${CC} ${CFLAGS} -Wall -o $@ $<

polycrypt:polycrypt.c
	${CC} ${CFLAGS} -Wall -o $@ $<
.PHONY:
clean:
	rm prog crypter_rt polycrypt
