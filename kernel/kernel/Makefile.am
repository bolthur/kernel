
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  debug.c \
  event.c \
  irq.c \
  main.c \
  panic.c
