# scc version
VERSION     = 0.1

# Customize below to fit your system

# paths
PREFIX    = /usr/local/
MANPREFIX = ${PREFIX}/share/man

CC = cc
LD = $(CC)
AR = ar

# for Plan9 add -D_SUSV2_SOURCE -DNBOOL
CPPFLAGS =
CFLAGS   = -std=c99
LDFLAGS  = -s
