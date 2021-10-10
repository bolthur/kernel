
AM_CFLAGS = -DPROGRAM_NAME=\"vfs\"

bin_PROGRAMS = vfs
vfs_SOURCES = \
  rpc/add.c \
  rpc/close.c \
  rpc/open.c \
  rpc/read.c \
  rpc/remove.c \
  rpc/seek.c \
  rpc/stat.c \
  rpc/write.c \
  avl.c \
  handle.c \
  list.c \
  main.c \
  util.c \
  vfs.c
vfs_LDFLAGS = -all-static --static -Wl,-s -Wl,--gc-sections
