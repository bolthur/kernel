
AM_CFLAGS = -DPROGRAM_NAME=\"fs/vfs\"

bin_PROGRAMS = vfs
vfs_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
vfs_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
vfs_SOURCES = \
  file/handle.c \
  ioctl/handler.c \
  rpc/add.c \
  rpc/close.c \
  rpc/exit.c \
  rpc/fork.c \
  rpc/init.c \
  rpc/ioctl.c \
  rpc/mount.c \
  rpc/open.c \
  rpc/read.c \
  rpc/remove.c \
  rpc/seek.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c \
  util.c \
  vfs.c
vfs_LDFLAGS = -all-static --static
