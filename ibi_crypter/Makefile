all: crypter_rt target1 target

crypter_rt: crypter_rt.c
	${CC} ${CFLAGS} -o $@ $<

target: target.c stub.c
	${CC} ${CFLAGS} -o $@ target.c stub.c
	./crypter_rt $@
	@echo "***********************************************"
	@echo "Update section address and size in stub.c!!!!!!!"
	@readelf -S $@ | grep "secure"

target1: target.c stub.c 
	${CC} ${CFLAGS} -o $@ target.c stub.c


.PHONY:
clean:
	rm crypter_rt target target1
