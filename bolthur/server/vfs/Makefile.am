
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"vfs\"

bin_PROGRAMS = vfs
vfs_SOURCES = \
  msg/add.c \
  msg/close.c \
  msg/open.c \
  msg/read.c \
  msg/remove.c \
  msg/seek.c \
  msg/stat.c \
  msg/write.c \
  avl.c \
  handle.c \
  list.c \
  main.c \
  msg.c \
  util.c \
  vfs.c
# -Wl,-s -Wl,--gc-sections necessary when debugging due to a startup weird behaviour
vfs_LDFLAGS = -all-static --static -Wl,-s -Wl,--gc-sections
