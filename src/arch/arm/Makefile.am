
SUBDIRS = ${subarch_subdir}

DIST_SUBDIRS = v6 v7 v8

noinst_LIBRARIES = libarm.a
libarm_a_SOURCES = \
  interrupt/interrupt.c \
  mm/virt.c \
  task/lock.c \
  arch.c \
  delay.c \
  elf.c \
  fdt.S \
  io.c \
  stack.c
