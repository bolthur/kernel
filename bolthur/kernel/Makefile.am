
ACLOCAL_AMFLAGS = -I ../../build-aux/m4

SUBDIRS = . thirdparty lib arch platform target

noinst_LTLIBRARIES = libkernel.la
libkernel_la_SOURCES = \
  debug/breakpoint.c \
  debug/gdb.c \
  debug/string.c \
  rpc/backup.c \
  rpc/data.c \
  rpc/generic.c \
  rpc/queue.c \
  mm/heap.c \
  mm/phys.c \
  mm/shared.c \
  mm/virt.c \
  syscall/init.c \
  syscall/interrupt.c \
  syscall/memory.c \
  syscall/print.c \
  syscall/rpc.c \
  syscall/task.c \
  syscall/timer.c \
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
  panic.c \
  timer.c
