all:infect target

infect: infect.c
	${CC} -o $@ $<

target: target.c
	${CC} -o $@ $<

.PHONY:
clean:
	rm -f infect target
