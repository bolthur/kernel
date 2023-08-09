
AM_CFLAGS = -DPROGRAM_NAME=\"storage/sd\"

bin_PROGRAMS = sd
sd_SOURCES = \
  rpc/init.c \
  rpc/read.c \
  rpc/write.c \
  emmc.c \
  main.c \
  sd.c \
  sdhost.c \
  util.c
sd_LDFLAGS = -all-static --static
