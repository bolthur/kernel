
AM_CFLAGS = -fPIC

bin_PROGRAMS = linker.so
linker_so_SOURCES = \
  arch/_${executable_format}/image.c \
  arch/${arch_subdir}/image.c \
  image.c \
  main.c
linker_so_LDFLAGS = -all-static -Wl,-static

# 32 bit start address
if IS32
  linker_so_LDFLAGS += -Wl,-Ttext=0x70000000
endif
# 64 bit start address
if IS64
  linker_so_LDFLAGS += -Wl,-Ttext=0x70000000
endif
