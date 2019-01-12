
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  barrier.c \
  debug.c \
  delay.c \
  irq.c \
  mmio.c
