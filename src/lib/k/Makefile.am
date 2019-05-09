
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include

noinst_LIBRARIES = libk.a
libk_a_SOURCES = \
  stdio/printf.c \
  stdio/putchar.c \
  stdio/puts.c \
  stdlib/itoa.c \
  stdlib/utoa.c \
  string/memchr.c \
  string/memcmp.c \
  string/memcpy.c \
  string/memmove.c \
  string/memset.c \
  string/strcmp.c \
  string/strlen.c \
  string/strrev.c
