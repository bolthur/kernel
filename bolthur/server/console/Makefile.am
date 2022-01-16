
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"console\"

bin_PROGRAMS = console
console_SOURCES = \
  handler/add.c \
  handler/select.c \
  rpc/write.c \
  console.c \
  handler.c \
  list.c \
  main.c
console_LDFLAGS = -all-static --static
