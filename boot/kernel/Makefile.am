
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/boot/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/boot/include

noinst_LIBRARIES = libkernel.a
libkernel_a_SOURCES =
