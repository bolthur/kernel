
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include

noinst_LIBRARIES = libfont.a
libfont_a_SOURCES = \
  font.8x8.c
