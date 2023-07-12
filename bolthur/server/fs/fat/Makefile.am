
bin_PROGRAMS = fat
fat_LDADD = \
  $(BFS_LIBS)
fat_CFLAGS = \
  -DPROGRAM_NAME=\"fs/fat\" \
  $(BFS_CFLAGS)
fat_SOURCES = \
  rpc/init.c \
  rpc/mount.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c \
  stat.c
fat_LDFLAGS = -all-static --static
