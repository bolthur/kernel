
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  stub/cpu.S \
  stub/irq.S \
  irq.c \
  isrs.c \
  ivt.c \
  start.S
