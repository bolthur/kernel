
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"console\"

bin_PROGRAMS = console
console_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
console_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
console_SOURCES = \
  rpc/custom/add.c \
  rpc/custom/input.c \
  rpc/custom/select.c \
  rpc/close.c \
  rpc/exec.c \
  rpc/exit.c \
  rpc/fork.c \
  rpc/init.c \
  rpc/open.c \
  rpc/read.c \
  rpc/write.c \
  console.c \
  handler.c \
  main.c \
  queue.c
console_LDFLAGS = -all-static --static
