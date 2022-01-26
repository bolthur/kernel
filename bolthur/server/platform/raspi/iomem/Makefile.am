
AM_CFLAGS = -DPROGRAM_NAME=\"iomem\"

bin_PROGRAMS = iomem
iomem_SOURCES = \
  rpc/mailbox.c \
  rpc/read_memory.c \
  rpc/write_memory.c \
  mailbox.c \
  main.c \
  mmio.c \
  property.c \
  rpc.c
iomem_LDFLAGS = -all-static --static
