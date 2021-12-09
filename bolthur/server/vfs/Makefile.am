
AM_CFLAGS = -DPROGRAM_NAME=\"vfs\"

bin_PROGRAMS = vfs
vfs_SOURCES = \
  collection/avl.c \
  collection/list.c \
  file/handle.c \
  ioctl/handler.c \
  rpc/add.c \
  rpc/close.c \
  rpc/exit.c \
  rpc/fork.c \
  rpc/ioctl.c \
  rpc/open.c \
  rpc/read.c \
  rpc/remove.c \
  rpc/seek.c \
  rpc/stat.c \
  rpc/write.c \
  main.c \
  util.c \
  vfs.c
vfs_LDFLAGS = -all-static --static
