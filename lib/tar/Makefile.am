
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include -I$(top_srcdir)/lib/tar/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include -I$(top_srcdir)/lib/tar/include

noinst_LIBRARIES = libtar.a
libtar_a_SOURCES = \
  tar.c
