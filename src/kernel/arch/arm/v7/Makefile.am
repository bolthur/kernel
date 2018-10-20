
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  stub/cpu.S \
  stub/irq.S \
  stub/ivt.S \
  cpu.c \
  cpu.h \
  isrs.c \
  ivt.c \
  start.S
