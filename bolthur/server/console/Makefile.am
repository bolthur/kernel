
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"console\"

bin_PROGRAMS = console
console_SOURCES = \
  handler/add.c \
  console.c \
  list.c \
  main.c
# -Wl,-s -Wl,--gc-sections necessary when debugging due to a startup weird behaviour
console_LDFLAGS = -all-static --static
