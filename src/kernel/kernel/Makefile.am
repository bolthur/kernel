
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES = \
  mm/heap.c \
  mm/phys.c \
  mm/placement.c \
  mm/virt.c \
  irq.c \
  main.c \
  panic.c
