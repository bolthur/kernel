
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = liblist.a
liblist_a_SOURCES = \
  construct.c \
  destruct.c \
  empty.c \
  lookup.c \
  node_create.c \
  peek.c \
  pop.c \
  print.c \
  push.c \
  remove.c
