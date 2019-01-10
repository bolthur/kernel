
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include -I$(top_srcdir)/lib/avl/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include -I$(top_srcdir)/lib/avl/include

noinst_LIBRARIES = libavl.a
libavl_a_SOURCES = \
  avl.c
