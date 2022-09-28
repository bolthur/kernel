
AM_CFLAGS = -DPROGRAM_NAME=\"storage/sd\"

bin_PROGRAMS = sd
sd_DEPENDENCIES = \
  ${abs_top_builddir}/../library/fs/libfs.la
sd_LDADD = \
  ${abs_top_builddir}/../library/fs/libfs.la
sd_SOURCES = \
  rpc/add.c \
  rpc/close.c \
  rpc/exit.c \
  rpc/fork.c \
  rpc/init.c \
  rpc/mount.c \
  rpc/open.c \
  rpc/read.c \
  rpc/remove.c \
  rpc/seek.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  emmc.c \
  main.c \
  sd.c \
  sdhost.c \
  util.c
sd_LDFLAGS = -all-static --static
