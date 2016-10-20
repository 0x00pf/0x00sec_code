all:proxy libproxify.so

proxy:proxy.c
	${CC} -o $@ $<

proxify.o:proxify.c
	${CC} -c -fpic -o $@ $<

libproxify.so:proxify.o
	${CC} -shared -o $@ $< -ldl

.PHONY:
clean:
	rm proxify.o libproxify.so proxy
