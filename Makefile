# scc - Suckless C Compiler

include config.mk

SUBDIRS  = \
	lib \
	cc1 \
	cc2

all clean:
	@echo scc build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@for i in ${SUBDIRS};\
	do \
		(cd $$i; ${MAKE} $(MAKEFLAGS) $@ || exit); \
	done;

.POSIX:
