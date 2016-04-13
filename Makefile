# scc - Suckless C Compiler
.POSIX:

include config.mk

DIRS  = lib cc1 cc2 driver/$(DRIVER)
ARCHS = z80 i386-sysv amd64-sysv qbe

all:
	for i in $(DIRS) ; \
	do  \
		(cd $$i && $(MAKE) -e -$(MAKEFLAGS)) ;\
	done
	cp -f cc1/cc1 bin/cc1
	cp -f cc2/cc2 bin/cc2
	cp -f driver/$(DRIVER)/scc bin/scc
	strip bin/cc1 bin/cc2 bin/scc

multi:
	cd lib && $(MAKE) -e $(MAKEFLAGS)
	for i in $(ARCHS) ; \
	do \
		$(MAKE) -$(MAKEFLAGS) $$i || exit ;\
	done

$(ARCHS):
	for i in cc1 cc2; \
	do \
		(cd $$i; \
		 ARCH=$@ ${MAKE} -e -$(MAKEFLAGS) clean ;\
		 ARCH=$@ $(MAKE) -e $$i || exit); \
	done
	ln -f cc1/cc1 bin/cc1-$@
	ln -f cc2/cc2 bin/cc2-$@
	strip bin/cc1-$@ bin/cc2-$@

install: all
	mkdir -p $(PREFIX)/libexec/scc/
	mkdir -p $(PREFIX)/bin/
	cp -f bin/cc* $(PREFIX)/libexec/scc/
	cp -f bin/cc1 $(PREFIX)/bin/cpp
	cp -f bin/scc $(PREFIX)/bin/
	cd $(PREFIX)/libexec/scc/ && chmod 755 cc*
	cd $(PREFIX)/bin && chmod 755 cpp scc

uninstall:
	rm -rf $(PREFIX)/libexec/scc/
	rm -f $(PREFIX)/bin/scc
	rm -f $(PREFIX)/bin/cpp

clean:
	for i in ${DIRS};\
	do \
		(cd $$i; ${MAKE} -$(MAKEFLAGS) $@ || exit); \
	done

multi-clean:
	for i in $(ARCHS) ; \
	do \
		ARCH=$$i $(MAKE) -e -$(MAKEFLAGS) clean || exit; \
	done

distclean: multi-clean
	rm -f bin/cc* bin/scc
