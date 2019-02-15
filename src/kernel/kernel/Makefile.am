
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  boot/bss.c \
  mm/heap.c \
  mm/phys.c \
  mm/placement.c \
  mm/virt.c \
  debug.c \
  event.c \
  irq.c \
  main.c \
  panic.c
