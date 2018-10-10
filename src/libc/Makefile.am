
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = libc.a
libc_a_SOURCES = \
  stdio/printf.c \
  stdio/putchar.c \
  stdio/puts.c \
  stdlib/abort.c \
  stdlib/calloc.c \
  stdlib/exit.c \
  stdlib/free.c \
  stdlib/itoa.c \
  stdlib/malloc.c \
  stdlib/realloc.c \
  stdlib/utoa.c \
  string/memcmp.c \
  string/memcpy.c \
  string/memmove.c \
  string/memset.c \
  string/strlen.c \
  string/strrev.c \
  sys/cdefs.h \
  stdio.h \
  stdlib.h \
  string.h
