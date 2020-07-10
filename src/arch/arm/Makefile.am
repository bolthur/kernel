
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AM_CFLAGS = -imacros ${top_builddir}/include/config.h
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  interrupt/interrupt.c \
  mm/virt.c \
  task/lock.c \
  arch.c \
  delay.c \
  elf.c \
  io.c \
  stack.c
