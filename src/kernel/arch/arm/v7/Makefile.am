
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  cpu/stub/cpu.S \
  cpu/cpu.c \
  interrupt/stub/irq.S \
  interrupt/stub/ivt.S \
  interrupt/ivt.c \
  arch.c \
  barrier.c \
  stack.S \
  start.S
