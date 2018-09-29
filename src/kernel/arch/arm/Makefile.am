
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  delay.c \
  delay.h \
  ivt.h \
  mmio.c \
  mmio.h
