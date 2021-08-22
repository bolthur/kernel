
AM_CFLAGS = -DPROGRAM_NAME=\"framebuffer\"

bin_PROGRAMS = framebuffer
framebuffer_SOURCES = \
  main.c \
  framebuffer.c
# FIXME: ADD AGAIN ONCE .la ISSUE IS FIXED IN SYSROOT
framebuffer_LDFLAGS = -all-static --static
framebuffer_LDADD = -lfreetype -lpng -lz
