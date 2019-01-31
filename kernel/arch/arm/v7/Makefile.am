
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  cpu/stub/cpu.S \
  cpu/cpu.c \
  interrupt/stub/irq.S \
  interrupt/stub/ivt.S \
  interrupt/ivt.c \
  mm/stub/initial.S \
  mm/stub/virt.S \
  mm/virt.c \
  arch.c \
  barrier.c \
  stack.S \
  start.S
