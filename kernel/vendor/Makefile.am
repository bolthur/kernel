
SUBDIRS = . ${vendor_subdir}

DIST_SUBDIRS = rpi

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel -I$(top_srcdir)/lib/stdc

noinst_LIBRARIES = libvendor.a
libvendor_a_SOURCES =
