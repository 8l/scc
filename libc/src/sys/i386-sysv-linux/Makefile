# See LICENSE file for copyright and license details.
.POSIX:

include ../../../../config.mk

SYSTEMINC = /dev/null
include $(SYSTEMINC)
SYSGENINC = /dev/null
include $(SYSGENINC)

GENASM = $(GENOBJ:.o=.s)
OBJ =

all: all.bootstrap
all.start: $(SYSOBJ)

clean: clean.bootstrap
clean.start:
	rm -f *.o $(GENASM) system.mk sysgen.mk

all.bootstrap clean.bootstrap: system.mk sysgen.mk
	$(MAKE) -e SYSTEMINC=system.mk SYSGENINC=sysgen.mk \
	        $(@:%.bootstrap=%.start)

system.mk: Makefile syscall.lst
	printf '%s ' 'SYSOBJ =' \
	       "$$(awk '/^#/{next}{printf "%s.o ", $$2}' syscall.lst)" \
	       $(OBJ) \
	> $@

sysgen.mk: syscall.lst
	printf '%s ' 'GENOBJ =' \
	       "$$(awk '/^#/{next}{printf "%s.o ", $$2}' syscall.lst)" \
	> $@

$(GENASM): syscall.lst
	awk -f syscall.awk < syscall.lst
