
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/external/dtc/libfdt
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/external/dtc/libfdt

# rpi library
noinst_LIBRARIES = librpi.a libfdt.a
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

libfdt_a_SOURCES =
libfdt.a$(EXEEXT):
	make --directory=${abs_srcdir}/../../../../external/dtc -f ${abs_srcdir}/../../../../external/dtc/Makefile clean
	PATH="$(PATH)" CC=$(CC) AR=$(AR) CFLAGS="$(cflags_libfdt)" make --directory=${abs_srcdir}/../../../../external/dtc -f ${abs_srcdir}/../../../../external/dtc/Makefile libfdt
	cp ${abs_srcdir}/../../../../external/dtc/libfdt/libfdt.a libfdt.a

# no installable programs
noinst_PROGRAMS = kernel.zwerg kernel.qemu kernel_qemu.bin ${output_img}

# qemu compilation with different linker script to get device tree passing working
kernel_qemu_SOURCES =
kernel_qemu_DEPENDENCIES = \
  kernel.${subarch_subdir}-qemu.ld \
  librpi.a \
  libfdt.a \
  ${top_builddir}/src/kernel/vendor/libvendor.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${top_builddir}/src/kernel/libkernel.a \
  ${top_builddir}/src/lib/avl/libavl.a \
  ${top_builddir}/src/lib/stdc/libstdc.a \
  ${top_builddir}/src/lib/tar/libtar.a
kernel_qemu_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.${subarch_subdir}-qemu.ld \
  -Wl,-Map,kernel.qemu.map \
  -Wl,-u,Entry

# Normal kernel building for physical device
kernel_zwerg_SOURCES =
kernel_zwerg_DEPENDENCIES = \
  kernel.${subarch_subdir}.ld \
  librpi.a \
  libfdt.a \
  ${top_builddir}/src/kernel/vendor/libvendor.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${top_builddir}/src/kernel/libkernel.a \
  ${top_builddir}/src/lib/avl/libavl.a \
  ${top_builddir}/src/lib/stdc/libstdc.a \
  ${top_builddir}/src/lib/tar/libtar.a
kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.${subarch_subdir}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

# Bunch of extra programs ( optional image kernel is copied to, passed during configure )
kernel_qemu_bin_SOURCES =
kernel_qemu.bin$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.qemu
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.qemu ${top_builddir}/src/kernel/vendor/rpi/kernel_qemu.sym
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.qemu
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.qemu -O binary ${top_builddir}/src/kernel/vendor/rpi/kernel_qemu

EXTRA_PROGRAMS = kernel.img kernel7.img kernel8.img
kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}

kernel7_img_SOURCES =
kernel7.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}

kernel8_img_SOURCES =
kernel8.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(MIST_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}
