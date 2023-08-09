
AM_CFLAGS = -DPROGRAM_NAME=\"terminal\"

bin_PROGRAMS = terminal
terminal_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
terminal_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
terminal_SOURCES = \
  main.c \
  output.c \
  psf.c \
  render.c \
  terminal.c \
  utf8.c
terminal_LDFLAGS = -all-static --static
