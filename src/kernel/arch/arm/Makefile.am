
SUBDIRS = . ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  boot/fpu.S \
  mm/virt.c \
  arch.c \
  barrier.c \
  delay.c \
  irq.c \
  mmio.c
