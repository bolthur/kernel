
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

noinst_LIBRARIES = libstdlib.a
libstdlib_a_SOURCES = \
  abort.c \
  aligned_alloc.c \
  calloc.c \
  free.c \
  malloc.c \
  realloc.c
