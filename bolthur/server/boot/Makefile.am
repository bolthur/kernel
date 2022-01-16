
AM_CFLAGS = -DPROGRAM_NAME=\"boot\"

bin_PROGRAMS = boot
boot_LDADD = \
  -lz \
  -ltar \
  -lfdt
boot_SOURCES = \
  main.c \
  ramdisk.c
boot_LDFLAGS = -all-static --static
