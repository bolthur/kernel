
AM_CFLAGS = -DPROGRAM_NAME=\"fs/dev\"

bin_PROGRAMS = dev
dev_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
dev_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/collection/avl/libavl.la
dev_SOURCES = \
  ioctl/handler.c \
  rpc/custom/kill.c \
  rpc/custom/start.c \
  rpc/watch/notify.c \
  rpc/watch/register.c \
  rpc/watch/release.c \
  rpc/add.c \
  rpc/init.c \
  rpc/ioctl.c \
  rpc/mount.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  handle.c \
  main.c \
  watch.c
dev_LDFLAGS = -all-static --static
