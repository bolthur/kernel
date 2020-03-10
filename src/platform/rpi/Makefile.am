
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

# rpi library
noinst_LIBRARIES = librpi.a

# normal rpi library
librpi_a_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib -I${top_srcdir}/thirdparty
librpi_a_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib -I${top_srcdir}/thirdparty
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
    boot/prepare.32.c \
    start.32.S
endif
if IS64
  librpi_a_SOURCES += \
    boot/start.64.S \
    boot/prepare.64.c \
    start.64.S
endif
