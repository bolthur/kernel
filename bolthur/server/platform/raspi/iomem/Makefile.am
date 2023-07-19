
AM_CFLAGS = -DPROGRAM_NAME=\"iomem\"

bin_PROGRAMS = iomem
iomem_SOURCES = \
  rpc/gpio/detect.c \
  rpc/gpio/event.c \
  rpc/gpio/function.c \
  rpc/gpio/lock.c \
  rpc/gpio/pull.c \
  rpc/gpio/status.c \
  rpc/gpio/unlock.c \
  rpc/mmio/lock.c \
  rpc/mmio/perform.c \
  rpc/mmio/unlock.c \
  rpc/mailbox.c \
  barrier.c \
  delay.c \
  dma.c \
  mailbox.c \
  main.c \
  mmio.c \
  property.c \
  rpc.c
iomem_LDFLAGS = -all-static --static
