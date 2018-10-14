
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl

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
