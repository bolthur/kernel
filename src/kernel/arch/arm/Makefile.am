
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  boot/mm/virt.c \
  boot/fpu.S \
  mm/v6/short.c \
  mm/v7/long.c \
  mm/v7/short.c \
  mm/virt.c \
  arch.c \
  barrier.c \
  delay.c \
  irq.c \
  mmio.c
