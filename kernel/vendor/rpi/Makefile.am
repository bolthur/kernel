
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/kernel/include -I$(top_srcdir)/lib/stdc/include

# rpi library
noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  irq.c \
  platform.c \
  peripheral.c \
  serial.c \
  start.S \
  timer.c \
  tty.c

# no installable programs
noinst_PROGRAMS = kernel.zwerg ${output_img}

# Normal kernel building for physical device
kernel_zwerg_SOURCES =
kernel_zwerg_DEPENDENCIES = \
  kernel.$(subarch_subdir).ld \
  librpi.a \
  ${top_builddir}/kernel/arch/${arch_subdir}/${subarch_subdir}/lib$(subarch_subdir).a \
  ${top_builddir}/kernel/arch/${arch_subdir}/lib$(arch_subdir).a \
  ${top_builddir}/kernel/kernel/libkernel.a \
  ${top_builddir}/lib/atag/libatag.a \
  ${top_builddir}/lib/avl/libavl.a \
  ${top_builddir}/lib/fdt/libfdt.a \
  ${top_builddir}/lib/stdc/libstdc.a \
  ${top_builddir}/lib/tar/libtar.a
kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/kernel/vendor/rpi/kernel.$(subarch_subdir).ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

EXTRA_PROGRAMS = kernel.img kernel7.img kernel8.img
kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/kernel/vendor/rpi/${output_img}

kernel7_img_SOURCES =
kernel7.img$(EXEEXT): ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/kernel/vendor/rpi/${output_img}

kernel8_img_SOURCES =
kernel8.img$(EXEEXT): ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/kernel/vendor/rpi/${output_img}
