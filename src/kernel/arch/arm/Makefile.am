
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  atag.c \
  atag.h \
  barrier.c \
  debug.c \
  debug.h \
  delay.c \
  delay.h \
  ivt.h \
  mmio.c \
  mmio.h
