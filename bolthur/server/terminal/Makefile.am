
bin_PROGRAMS = terminal
terminal_CFLAGS = \
  -DPROGRAM_NAME=\"terminal\" \
  $(CONFINI_CFLAGS) \
  $(FREETYPE_CFLAGS)
terminal_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
terminal_LDADD = \
  $(FREETYPE_LIBS) \
  $(CONFINI_LIBS) \
  ${abs_top_builddir}/../library/collection/list/liblist.la \
  ${abs_top_builddir}/../library/vfs/libvfs.la
terminal_SOURCES = \
  rpc/custom/bootstrap.c \
  rpc/default/ioctl.c \
  rpc/init.c \
  buffer.c \
  main.c \
  output.c \
  psf.c \
  render.c \
  terminal.c \
  ttf.c \
  utf8.c
terminal_LDFLAGS = -all-static --static
