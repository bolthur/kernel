
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/include

# rpi library
noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  mm/virt.c \
  framebuffer.c \
  irq.c \
  platform.c \
  peripheral.c \
  serial.c \
  start.$(executable_format).S \
  timer.c \
  tty.c

# built sources determining things to be built first
BUILT_SOURCES = \
  ${top_builddir}/src/boot/vendor/${vendor_subdir}/librpi.a \
  ${top_builddir}/src/boot/arch/${arch_subdir}/${subarch_subdir}/lib$(subarch_subdir).a \
  ${top_builddir}/src/boot/arch/${arch_subdir}/lib$(arch_subdir).a \
  ${top_builddir}/src/boot/boot/libboot.a \
  ${top_builddir}/src/font/libfont.a \
  librpi.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib$(subarch_subdir).a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib$(arch_subdir).a \
  ${top_builddir}/src/kernel/kernel/libkernel.a \
  ${top_builddir}/src/lib/atag/libatag.a \
  ${top_builddir}/src/lib/avl/libavl.a \
  ${top_builddir}/src/lib/fdt/libfdt.a \
  ${top_builddir}/src/lib/stdc/libstdc.a \
  ${top_builddir}/src/lib/tar/libtar.a \
  kernel.zwerg

# no installable programs
noinst_PROGRAMS = kernel.zwerg ${output_img}

# Normal kernel building for physical device
kernel_zwerg_SOURCES =
kernel_zwerg_DEPENDENCIES = \
  kernel.$(subarch_subdir).$(executable_format).ld \
  ${top_builddir}/src/boot/vendor/${vendor_subdir}/librpi.a \
  ${top_builddir}/src/boot/arch/${arch_subdir}/${subarch_subdir}/lib$(subarch_subdir).a \
  ${top_builddir}/src/boot/arch/${arch_subdir}/lib$(arch_subdir).a \
  ${top_builddir}/src/boot/boot/libboot.a \
  ${top_builddir}/src/font/libfont.a \
  librpi.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib$(subarch_subdir).a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib$(arch_subdir).a \
  ${top_builddir}/src/kernel/kernel/libkernel.a \
  ${top_builddir}/src/lib/atag/libatag.a \
  ${top_builddir}/src/lib/avl/libavl.a \
  ${top_builddir}/src/lib/fdt/libfdt.a \
  ${top_builddir}/src/lib/stdc/libstdc.a \
  ${top_builddir}/src/lib/tar/libtar.a
kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.$(subarch_subdir).$(executable_format).ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

EXTRA_PROGRAMS = kernel.img kernel7.img kernel8.img
kernel_img_DEPENDENCIES = kernel.zwerg
kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}

kernel7_img_DEPENDENCIES = kernel.zwerg
kernel7_img_SOURCES =
kernel7.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}

kernel8_img_DEPENDENCIES = kernel.zwerg
kernel8_img_SOURCES =
kernel8.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg ${top_builddir}/src/kernel/vendor/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
	$(BOLTHUR_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/${output_img}
