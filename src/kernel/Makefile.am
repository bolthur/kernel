
SUBDIRS = . arch vendor
AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  debug.c \
  debug.h \
  isrs.h \
  isrs.c \
  irq.h \
  irq.c \
  irq.h \
  main.c \
  panic.c \
  panic.h \
  platform.h \
  serial.h \
  timer.h \
  tty.h
