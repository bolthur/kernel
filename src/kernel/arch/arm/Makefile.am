
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  atag.c \
  atag.h \
  barrier.c \
  debug.c \
  debug.h \
  delay.c \
  delay.h \
  fdt.c \
  fdt.h \
  ivt.h \
  mmio.c \
  mmio.h
