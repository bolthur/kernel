
AM_CFLAGS = -DPROGRAM_NAME=\"mount\"

bin_PROGRAMS = mount
mount_LDADD = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
mount_DEPENDENCIES = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
mount_SOURCES = \
  rpc/init.c \
  rpc/mount.c \
  rpc/umount.c \
  main.c
mount_LDFLAGS = -all-static --static
