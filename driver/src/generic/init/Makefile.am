
AM_CFLAGS = -DPROGRAM_NAME=\"init\"

bin_PROGRAMS = init
init_LDADD = \
  -lz \
  -ltar \
  -lfdt
init_SOURCES = \
  main.c \
  ramdisk.c
init_LDFLAGS = -all-static --static
