# See LICENSE file for copyright and license details.
.POSIX:

include ../../config.mk

SYSDIR = sys/$(ARCH)-$(SYS)
SYSINC = $(SYSDIR)/system.mk

SYSINCLUDE = /dev/null
include $(SYSINCLUDE)

LIBCOBJ = assert.o strcpy.o strcmp.o strlen.o strchr.o \
          strrchr.o strcat.o strncmp.o strncpy.o strncat.o strcoll.o \
          strxfrm.o strstr.o strspn.o strcspn.o strpbrk.o strtok.o \
          memset.o memcpy.o memmove.o memcmp.o memchr.o \
          isalnum.o isalpha.o isascii.o isblank.o iscntrl.o isdigit.o \
          isgraph.o islower.o isprint.o ispunct.o isspace.o isupper.o \
          isxdigit.o toupper.o tolower.o ctype.o setlocale.o \
          localeconv.o atoi.o atexit.o exit.o \
          printf.o fprintf.o vfprintf.o \
          realloc.o calloc.o malloc.o
LIBCSYSOBJ = $(SYSOBJ:%=$(SYSDIR)/%)
OBJ = $(LIBCOBJ) $(LIBCSYSOBJ)

all: all.bootstrap
all.start: libc.a

clean: clean.bootstrap
clean.start:
	rm -f *.o libc.a
	cd $(SYSDIR) && $(MAKE) clean

all.bootstrap clean.bootstrap: $(SYSINC)
	$(MAKE) -e SYSINCLUDE="$(SYSINC)" $(@:%.bootstrap=%.start)

$(SYSINC): $(SYSDIR)/Makefile $(SYSDIR)/syscall.lst
	cd $(SYSDIR) && $(MAKE) -e system.mk

libc.a: $(SYSINC) $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)
	ranlib $@

$(LIBCSYSOBJ):
	cd $(SYSDIR) && $(MAKE)
