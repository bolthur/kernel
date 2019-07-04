
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include -I$(top_srcdir)/include/lib
AM_CCASFLAGS = -imacros $(top_builddir)/include/config.h -I$(top_srcdir)/include -I$(top_srcdir)/include/lib

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  boot/mm/virt/long.c \
  boot/mm/virt/short.c \
  boot/mm/virt.c \
  cpu/stub/cpu.S \
  cpu/cpu.c \
  interrupt/stub/irq.S \
  interrupt/stub/ivt.S \
  interrupt/ivt.c \
  mm/virt/long.c \
  mm/virt/short.c \
  mm/virt.c \
  arch.c \
  barrier.c \
  stack.S \
  start.S
