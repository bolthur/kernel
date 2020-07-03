
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = libstdio.a
libstdio_a_SOURCES = \
  printf.c \
  putchar.c \
  puts.c \
  sprintf.c \
  vprintf.c \
  vsprintf.c
