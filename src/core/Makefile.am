
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

noinst_LIBRARIES = libcore.a
libcore_a_SOURCES = \
  boot/bss.c \
  interrupt/interrupt.c \
  mm/heap.c \
  mm/phys.c \
  mm/placement.c \
  mm/virt.c \
  smp/lock.c \
  syscall/init.c \
  task/process.c \
  task/queue.c \
  task/stack.c \
  task/thread.c \
  cpu.c \
  elf.c \
  event.c \
  initrd.c \
  main.c \
  panic.c
