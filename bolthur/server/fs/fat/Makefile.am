
bin_PROGRAMS = fat
fat_LDADD = \
  $(BFS_LIBS) \
  ${abs_top_builddir}/../library/handle/libhandle.la
fat_DEPENDENCIES = \
  ${abs_top_builddir}/../library/handle/libhandle.la
fat_CFLAGS = \
  -DPROGRAM_NAME=\"fs/fat\" \
  $(BFS_CFLAGS)
fat_SOURCES = \
  rpc/close.c \
  rpc/getdents.c \
  rpc/init.c \
  rpc/mount.c \
  rpc/open.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c \
  stat.c
fat_LDFLAGS = -all-static --static
