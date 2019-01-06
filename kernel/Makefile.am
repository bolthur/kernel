
SUBDIRS = . arch vendor
AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  debug.c \
  debug.h \
  event.c \
  event.h \
  irq.h \
  irq.c \
  main.c \
  panic.c \
  panic.h \
  platform.h \
  serial.h \
  timer.h \
  tty.h
