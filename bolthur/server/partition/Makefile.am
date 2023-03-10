
AM_CFLAGS = -DPROGRAM_NAME=\"partition\"

bin_PROGRAMS = partition
partition_SOURCES = \
  rpc/custom/register.c \
  rpc/custom/release.c \
  rpc/watch/notify.c \
  rpc/init.c \
  rpc/mount.c \
  handler.c \
  main.c \
  mount.c \
  partition.c
partition_LDFLAGS = -all-static --static
