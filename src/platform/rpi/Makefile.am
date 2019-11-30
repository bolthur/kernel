
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

# rpi library
noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  boot/mm/virt.c \
  interrupt/interrupt.c \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  mm/virt.c \
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
