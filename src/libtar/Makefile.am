
AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar

noinst_LIBRARIES = libtar.a
libtar_a_SOURCES = tar.h \
  tar.c
