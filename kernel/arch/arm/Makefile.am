
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  barrier.c \
  barrier.h \
  debug.c \
  debug.h \
  delay.c \
  delay.h \
  irq.c \
  ivt.h \
  mmio.c \
  mmio.h
