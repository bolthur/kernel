
bin_PROGRAMS = ext
ext_LDADD = \
  $(BFS_LIBS)
ext_CFLAGS = \
  -DPROGRAM_NAME=\"fs/ext\" \
  $(BFS_CFLAGS)
ext_SOURCES = \
  rpc/getdents.c \
  rpc/init.c \
  rpc/mount.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c \
  stat.c
ext_LDFLAGS = -all-static --static
