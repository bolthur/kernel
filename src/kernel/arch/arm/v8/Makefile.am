
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar

noinst_LIBRARIES = libv8.a
libv8_a_SOURCES =
