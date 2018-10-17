
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc -I$(top_srcdir)/src/libavl -I$(top_srcdir)/src/libfdt -I$(top_srcdir)/src/libtar

noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  gpio.h \
  irq.c \
  mailbox.c \
  mailbox.h \
  platform.c \
  platform.h \
  peripheral.h \
  serial.c \
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
  ${top_builddir}/src/libavl/libavl.a \
  ${top_builddir}/src/libc/libc.a \
  ${top_builddir}/src/libfdt/libfdt.a \
  ${top_builddir}/src/libtar/libtar.a

kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.${subarch_subdir}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_name}.sym
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_name}.img
