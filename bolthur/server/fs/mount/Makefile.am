
AM_CFLAGS = -DPROGRAM_NAME=\"fs/mount\"

bin_PROGRAMS = mount
mount_DEPENDENCIES = \
  ${abs_top_builddir}/../library/fs/libfs.la
mount_LDADD = \
  ${abs_top_builddir}/../library/fs/libfs.la
mount_SOURCES = \
  rpc/custom/auto_mount.c \
  rpc/custom/mount.c \
  rpc/custom/unmount.c \
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
  main.c
mount_LDFLAGS = -all-static --static
