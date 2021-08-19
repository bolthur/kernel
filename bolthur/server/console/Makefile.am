
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"console\"

bin_PROGRAMS = console
console_SOURCES = \
  main.c
# -Wl,-s -Wl,--gc-sections necessary when debugging due to a startup weird behaviour
console_LDFLAGS = -all-static --static
