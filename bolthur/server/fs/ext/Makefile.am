
bin_PROGRAMS = ext
ext_LDADD = \
  $(BFS_LIBS) \
  ${abs_top_builddir}/../library/handle/libhandle.la
ext_DEPENDENCIES = \
  ${abs_top_builddir}/../library/handle/libhandle.la
ext_CFLAGS = \
  -DPROGRAM_NAME=\"fs/ext\" \
  $(BFS_CFLAGS)
ext_SOURCES = \
  rpc/close.c \
  rpc/exec.c \
  rpc/fork.c \
  rpc/getdents.c \
  rpc/exit.c \
  rpc/init.c \
  rpc/mount.c \
  rpc/open.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c \
  stat.c
ext_LDFLAGS = -all-static --static
