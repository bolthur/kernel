
AM_CFLAGS = -DPROGRAM_NAME=\"storage/emmc\"

bin_PROGRAMS = emmc
emmc_SOURCES = \
  helper/clock.c \
  helper/command.c \
  helper/interrupt.c \
  helper/power.c \
  helper/reset.c \
  helper/version.c \
  emmc.c \
  main.c \
  util.c
emmc_LDFLAGS = -all-static --static
