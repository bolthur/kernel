
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

noinst_LIBRARIES = libv6.a
libv6_a_SOURCES = \
  boot/mm/virt/short.c \
  boot/mm/virt.c \
  mm/virt/short.c \
  mm/virt.c \
  barrier.c
