
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = libstring.a
libstring_a_SOURCES = \
  memcmp.c \
  memcpy.c \
  memmove.c \
  memset.c \
  strchr.c \
  strlen.c \
  strncmp.c
