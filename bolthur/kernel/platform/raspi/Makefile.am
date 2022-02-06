
# raspi library
noinst_LTLIBRARIES = libraspi.la

# normal raspi library
libraspi_la_SOURCES = \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/dma.c \
  mm/phys.c \
  mm/virt.c \
  interrupt.c \
  peripheral.c \
  serial.c \
  timer.c \
  tty.c
if IS32
  libraspi_la_SOURCES += \
    start.32.S
endif
if IS64
  libraspi_la_SOURCES += \
    start.64.S
endif
