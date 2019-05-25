
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib

noinst_LIBRARIES = libstdio.a
libstdio_a_SOURCES = \
  printf.c \
  putchar.c \
  puts.c
