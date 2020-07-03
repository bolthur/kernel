
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = libstdlib.a
libstdlib_a_SOURCES = \
  abort.c \
  aligned_alloc.c \
  calloc.c \
  free.c \
  malloc.c \
  realloc.c
