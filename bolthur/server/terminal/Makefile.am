
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"terminal\"

bin_PROGRAMS = terminal
terminal_SOURCES = \
  main.c
# -Wl,-s -Wl,--gc-sections necessary when debugging due to a startup weird behaviour
terminal_LDFLAGS = -all-static --static
