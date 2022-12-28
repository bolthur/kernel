
bin_PROGRAMS = ext
ext_LDADD = \
  $(LWEXT4_LIBS)
ext_CFLAGS = \
  -DPROGRAM_NAME=\"fs/ext\" \
  $(LWEXT4_CFLAGS)
ext_SOURCES = \
  rpc/init.c \
  rpc/mount.c \
  rpc/read.c \
  rpc/stat.c \
  rpc/umount.c \
  rpc/write.c \
  main.c
