
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  mm/virt.c \
  start.$(executable_format).S
