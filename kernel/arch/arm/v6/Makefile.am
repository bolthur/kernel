
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc

noinst_LIBRARIES = libv6.a
libv6_a_SOURCES = \
  barrier.c
