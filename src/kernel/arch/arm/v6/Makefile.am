
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian

noinst_LIBRARIES = libv6.a
libv6_a_SOURCES =
