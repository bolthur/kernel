
SUBDIRS = . ${vendor_subdir}

DIST_SUBDIRS = beagleboard n8x0 n900 rpi

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl

noinst_LIBRARIES = libvendor.a
libvendor_a_SOURCES =
