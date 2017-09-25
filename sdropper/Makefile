all:poc-alt poc-net

poc-alt: poc-alt.c
	${CC} -o $@ $< -lrt

poc-net: poc-net.c
	${CC} -o $@ $<

.PHONY:
clean:
	rm -f poc-alt poc-net
