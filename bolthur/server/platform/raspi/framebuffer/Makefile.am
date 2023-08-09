
AM_CFLAGS = -DPROGRAM_NAME=\"framebuffer\"

bin_PROGRAMS = framebuffer
framebuffer_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
framebuffer_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
framebuffer_SOURCES = \
  framebuffer.c \
  main.c
framebuffer_LDFLAGS = -all-static --static
