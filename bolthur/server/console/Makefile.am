
# -Os necessary when debugging due to a weird startup behaviour
AM_CFLAGS = -DPROGRAM_NAME=\"console\"

bin_PROGRAMS = console
console_DEPENDENCIES = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
console_LDADD = \
  ${abs_top_builddir}/../library/collection/list/liblist.la
console_SOURCES = \
  rpc/custom/add.c \
  rpc/custom/select.c \
  rpc/init.c \
  rpc/write.c \
  console.c \
  main.c
console_LDFLAGS = -all-static --static
