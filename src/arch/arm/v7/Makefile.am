
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

noinst_LIBRARIES = libv7.a
libv7_a_SOURCES = \
  boot/mm/virt/long.c \
  boot/mm/virt/short.c \
  boot/mm/virt.c \
  boot/fpu.S \
  debug/breakpoint.c \
  debug/debug.c \
  debug/disasm.c \
  debug/gdb.c \
  interrupt/monitor/handler/data.c \
  interrupt/monitor/handler/fiq.c \
  interrupt/monitor/handler/irq.c \
  interrupt/monitor/handler/prefetch.c \
  interrupt/monitor/handler/smc.c \
  interrupt/monitor/handler/unknown.c \
  interrupt/monitor/stub/vector.S \
  interrupt/stub/interrupt.S \
  interrupt/vector/handler/data.c \
  interrupt/vector/handler/fiq.c \
  interrupt/vector/handler/irq.c \
  interrupt/vector/handler/prefetch.c \
  interrupt/vector/handler/svc.c \
  interrupt/vector/handler/undefined.c \
  interrupt/vector/stub/vector.S \
  interrupt/monitor.c \
  interrupt/vector.c \
  mm/virt/long.c \
  mm/virt/short.c \
  mm/virt.c \
  stub/stack.S \
  stub/start.S \
  syscall/putc.c \
  task/stub/thread.S \
  task/process.c \
  task/stack.c \
  task/thread.c \
  arch.c \
  barrier.c \
  cache.c
