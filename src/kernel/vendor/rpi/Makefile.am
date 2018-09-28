
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  serial.c \
  gpio.h \
  gpio.c \
  peripheral.h \
  platform.c \
  start.S \
  timer.c \
  tty.c

noinst_PROGRAMS = kernel.zwerg kernel.img
kernel_zwerg_SOURCES =

kernel_zwerg_DEPENDENCIES = \
  kernel.${subarch_subdir}.ld \
  librpi.a \
  ${top_builddir}/src/kernel/vendor/libvendor.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${top_builddir}/src/kernel/libkernel.a \
  ${top_builddir}/src/libc/libc.a

kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.${subarch_subdir}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/kernel.sym
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/kernel.img
