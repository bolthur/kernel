
AM_CFLAGS = -fPIC

bin_PROGRAMS = ld-bolthur.so
ld_bolthur_so_SOURCES = \
  arch/${arch_subdir}/image.c \
  bit/${executable_format}/image.c \
  image.c \
  list.c \
  main.c
ld_bolthur_so_LDFLAGS = -all-static -Wl,-static -Wl,-s -Wl,--gc-sections
# 32 bit start address
if IS32
  ld_bolthur_so_LDFLAGS += -Wl,-Ttext-segment=0x70000000
endif
# 64 bit start address
if IS64
  ld_bolthur_so_LDFLAGS += -Wl,-Ttext-segment=0x70000000
endif
