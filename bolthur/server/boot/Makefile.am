
AM_CFLAGS = -DPROGRAM_NAME=\"boot\"

bin_PROGRAMS = boot
boot_LDADD = \
  -lz \
  -ltar \
  -lfdt \
  -lconfini
boot_SOURCES = \
  init/stage1.c\
  init/stage2.c\
  init/stage3.c\
  main.c \
  ramdisk.c \
  util.c
boot_LDFLAGS = -all-static --static
