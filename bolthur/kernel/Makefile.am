
ACLOCAL_AMFLAGS = -I ../../build-aux/m4

SUBDIRS = . thirdparty lib arch platform target

noinst_LTLIBRARIES = libkernel.la
libkernel_la_SOURCES = \
  debug/breakpoint.c \
  debug/gdb.c \
  debug/string.c \
  ipc/message.c \
  ipc/signal.c \
  mm/heap.c \
  mm/phys.c \
  mm/shared.c \
  mm/virt.c \
  syscall/print.c \
  syscall/init.c \
  syscall/interrupt.c \
  syscall/memory.c \
  syscall/message.c \
  syscall/task.c \
  task/lock.c \
  task/process.c \
  task/queue.c \
  task/stack.c \
  task/thread.c \
  bss.c \
  cpu.c \
  elf.c \
  event.c \
  initrd.c \
  interrupt.c \
  main.c \
  panic.c
