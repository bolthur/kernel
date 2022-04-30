
AM_CFLAGS = -DPROGRAM_NAME=\"storage/sd\" $(BOSL_CFLAGS)

bin_PROGRAMS = sd
sd_LDADD = $(BOSL_LIBS)
sd_SOURCES = \
  emmc.c \
  main.c \
  sd.c \
  sdhost.c \
  util.c
sd_LDFLAGS = -all-static --static
