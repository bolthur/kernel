
AM_CFLAGS = -DPROGRAM_NAME=\"fs/ramdisk\"

bin_PROGRAMS = ramdisk
ramdisk_LDADD = -ltar \
  ${abs_top_builddir}/../library/vfs/libvfs.la
ramdisk_DEPENDENCIES = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
ramdisk_SOURCES = \
  rpc/add.c \
  rpc/close.c \
  rpc/exec.c \
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
  main.c \
  ramdisk.c
ramdisk_LDFLAGS = -all-static --static
