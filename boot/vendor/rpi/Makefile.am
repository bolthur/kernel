
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/boot/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/boot/include

# rpi library
noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  start.$(executable_format).S
