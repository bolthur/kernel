
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include/kernel -I$(top_srcdir)/include/lib -I$(top_srcdir)/include

noinst_LIBRARIES = libv6.a
libv6_a_SOURCES = \
  mm/virt/short.c \
  mm/virt.c \
  barrier.c
