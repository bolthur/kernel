
AM_CFLAGS = -fPIC -I$(srcdir)/include -DPROGRAM_NAME=\"ld-bolthur\"

bin_PROGRAMS = ld-bolthur.so
ld_bolthur_so_SOURCES = \
  src/tmp/dl.c \
  src/tmp/stub.S \
  src/main.c
ld_bolthur_so_LDFLAGS = -all-static --static
# 32 bit start address
if IS32
  ld_bolthur_so_LDFLAGS += -Wl,-Ttext-segment=0x70000000
endif
# 64 bit start address
if IS64
  # FIXME: Specify correct text segment address
  ld_bolthur_so_LDFLAGS += -Wl,-Ttext-segment=0x70000000
endif
