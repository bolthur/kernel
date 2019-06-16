
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include

noinst_LIBRARIES = libstdlib.a
libstdlib_a_SOURCES = \
  abort.c \
  calloc.c \
  free.c \
  itoa.c \
  malloc.c \
  realloc.c \
  utoa.c
