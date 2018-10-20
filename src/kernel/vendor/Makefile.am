
SUBDIRS = . ${vendor_subdir}

DIST_SUBDIRS = beagleboard n8x0 n900 rpi

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian

noinst_LIBRARIES = libvendor.a
libvendor_a_SOURCES =
