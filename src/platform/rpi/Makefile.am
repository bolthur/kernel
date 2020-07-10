
SUBDIRS = .

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/thirdparty
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/thirdparty

# rpi library
noinst_LIBRARIES = librpi.a

# normal rpi library
librpi_a_SOURCES = \
  boot/mm/virt.c \
  interrupt/interrupt.c \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  mm/virt.c \
  framebuffer.c \
  initrd.c \
  platform.c \
  peripheral.c \
  serial.c \
  timer.c \
  tty.c
if IS32
  librpi_a_SOURCES += \
    boot/start.32.S \
    start.32.S
endif
if IS64
  librpi_a_SOURCES += \
    boot/start.64.S \
    start.64.S
endif
