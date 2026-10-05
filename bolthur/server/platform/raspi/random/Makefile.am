
AM_CFLAGS = -DPROGRAM_NAME=\"random\"

bin_PROGRAMS = random
random_DEPENDENCIES = \
  ${abs_top_builddir}/../library/platform/raspi/iomem/libiomem.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
random_LDADD = \
  ${abs_top_builddir}/../library/platform/raspi/iomem/libiomem.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
random_SOURCES = \
  rpc/init.c \
  rpc/read.c \
  main.c \
  random.c
random_LDFLAGS = -all-static --static
