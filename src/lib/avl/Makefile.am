
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = libavl.a
libavl_a_SOURCES =\
  balance.c \
  create.c \
  destroy.c \
  find.c \
  insert.c \
  max.c \
  min.c \
  prepare.c \
  print.c \
  remove.c
