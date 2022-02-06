
AM_CFLAGS = -DPROGRAM_NAME=\"iomem\"

bin_PROGRAMS = iomem
iomem_SOURCES = \
  rpc/lock.c \
  rpc/mailbox.c \
  rpc/mmio.c \
  rpc/unlock.c \
  delay.c \
  mailbox.c \
  main.c \
  mmio.c \
  property.c \
  rpc.c
iomem_LDFLAGS = -all-static --static
