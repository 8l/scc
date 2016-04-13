# scc version
VERSION     = 0.1

# Customize below to fit your system
ARCH = z80
DRIVER = posix

# paths
PREFIX    = $(HOME)
MANPREFIX = ${PREFIX}/share/man

# if your system is not POSIX maybe you want to use cc or gcc
# CC = c99
# AR = ar

# for Plan9 add -D_SUSV2_SOURCE -DNBOOL
SCC_CFLAGS = -DNDEBUG -Iarch/$(ARCH) -DPREFIX=\"$(PREFIX)\" $(CFLAGS)
SCC_LDFLAGS  = $(LDFLAGS)

.c.o:
	$(CC) $(SCC_CFLAGS) -o $@ -c $<

.c:
	$(CC) $(SCC_CFLAGS) $(SCC_LDFLAGS) -o $@ $<
