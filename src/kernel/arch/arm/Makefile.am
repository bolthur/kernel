
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar

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
