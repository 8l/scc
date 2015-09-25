# scc version
VERSION     = 0.1

# Customize below to fit your system
ARCH = z80

# paths
PREFIX    = /usr/local/
MANPREFIX = ${PREFIX}/share/man

CC = cc
LD = $(CC)
AR = ar

# for Plan9 add -D_SUSV2_SOURCE -DNBOOL
CFLAGS   = -O2 -std=c99
LDFLAGS  = -s
CPPFLAGS = -DNDEBUG -Iarch/$(ARCH)
