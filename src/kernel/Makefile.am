
SUBDIRS = . arch vendor
AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  barrier.h \
  debug.c \
  debug.h \
  event.c \
  event.h \
  irq.h \
  irq.c \
  isrs.c \
  isrs.h \
  main.c \
  panic.c \
  panic.h \
  platform.h \
  serial.h \
  timer.h \
  tty.h
