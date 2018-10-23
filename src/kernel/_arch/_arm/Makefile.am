
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = _v6 _v7 _v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/external/dtc/libfdt
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/external/dtc/libfdt

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  atag.c \
  atag.h \
  barrier.c \
  barrier.h \
  debug.c \
  debug.h \
  delay.c \
  delay.h \
  fdt.c \
  fdt.h \
  irq.c \
  ivt.h \
  mmio.c \
  mmio.h
