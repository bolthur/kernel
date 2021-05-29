
AM_CFLAGS = -fPIC

bin_PROGRAMS = linker.so
linker_so_SOURCES = \
  main.c
# link as static binary and load to end of user space
linker_so_LDFLAGS = -all-static -Wl,-static -Wl,-Ttext=0x70000000
