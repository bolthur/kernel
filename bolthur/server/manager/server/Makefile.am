
AM_CFLAGS = -DPROGRAM_NAME=\"manager/server\"

bin_PROGRAMS = server
server_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
server_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
server_SOURCES = \
  ioctl/handler.c \
  rpc/custom/start.c \
  rpc/add.c \
  rpc/close.c \
  rpc/exit.c \
  rpc/fork.c \
  rpc/init.c \
  rpc/ioctl.c \
  rpc/open.c \
  rpc/read.c \
  rpc/remove.c \
  rpc/seek.c \
  rpc/stat.c \
  rpc/write.c \
  handle.c \
  main.c
server_LDFLAGS = -all-static --static
