
bin_PROGRAMS = boot
boot_CFLAGS = \
  -DPROGRAM_NAME=\"boot\" \
  $(CONFINI_CFLAGS) \
  $(ZLIB_CFLAGS)
boot_LDADD = \
  $(CONFINI_LIBS) \
  $(ZLIB_LIBS) \
  -ltar \
  -lfdt
boot_SOURCES = \
  init/stage1.c \
  init/stage2.c \
  init/stage3.c \
  ramdisk/dump.c \
  ramdisk/extract.c \
  ramdisk/lookup.c \
  ramdisk/size.c \
  main.c \
  util.c
boot_LDFLAGS = -all-static --static
