
# rpi library
noinst_LTLIBRARIES = librpi.la

# normal rpi library
librpi_la_SOURCES = \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  mm/virt.c \
  framebuffer.c \
  interrupt.c \
  peripheral.c \
  serial.c \
  timer.c \
  tty.c
if IS32
  librpi_la_SOURCES += \
    start.32.S
endif
if IS64
  librpi_la_SOURCES += \
    start.64.S
endif
