
AM_CFLAGS = -DPROGRAM_NAME=\"storage/sd\"

bin_PROGRAMS = sd
sd_SOURCES = \
  rpc/init.c \
  rpc/mount.c \
  rpc/read.c \
  rpc/umount.c \
  emmc.c \
  main.c \
  sd.c \
  sdhost.c \
  util.c
sd_LDFLAGS = -all-static --static
