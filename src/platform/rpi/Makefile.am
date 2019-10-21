
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib
AM_CCASFLAGS = -imacros ${top_builddir}/include/config.h -I${top_srcdir}/include -I${top_srcdir}/include/lib

# rpi library
noinst_LIBRARIES = librpi.a
librpi_a_SOURCES = \
  boot/mm/virt.c \
  mailbox/mailbox.c \
  mailbox/property.c \
  mm/phys.c \
  mm/virt.c \
  initrd.c \
  irq.c \
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

# built sources determining things to be built first
BUILT_SOURCES = \
  librpi.a \
  ${abs_top_builddir}/src/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${abs_top_builddir}/src/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${abs_top_builddir}/src/kernel/libkernel.a \
  ${abs_top_builddir}/src/lib/avl/libavl.a \
  ${abs_top_builddir}/src/lib/list/liblist.a \
  ${abs_top_builddir}/src/lib/stdio/libstdio.a \
  ${abs_top_builddir}/src/lib/stdlib/libstdlib.a \
  ${abs_top_builddir}/src/lib/string/libstring.a \
  ${abs_top_builddir}/src/lib/ubsan/libubsan.a \
  ${abs_top_builddir}/src/lib/tar/libtar.a \
  kernel.elf

# no installable programs
noinst_PROGRAMS = kernel.elf ${output_img}

# Normal kernel building for physical device
kernel_elf_SOURCES =
kernel_elf_DEPENDENCIES = \
  kernel.${subarch_subdir}.${executable_format}.ld \
  librpi.a \
  ${abs_top_builddir}/src/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${abs_top_builddir}/src/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${abs_top_builddir}/src/kernel/libkernel.a \
  ${abs_top_builddir}/src/lib/avl/libavl.a \
  ${abs_top_builddir}/src/lib/list/liblist.a \
  ${abs_top_builddir}/src/lib/stdio/libstdio.a \
  ${abs_top_builddir}/src/lib/stdlib/libstdlib.a \
  ${abs_top_builddir}/src/lib/string/libstring.a \
  ${abs_top_builddir}/src/lib/ubsan/libubsan.a \
  ${abs_top_builddir}/src/lib/tar/libtar.a
kernel_elf_LDFLAGS = \
  -T ${top_srcdir}/src/platform/rpi/kernel.${subarch_subdir}.${executable_format}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

EXTRA_PROGRAMS = kernel.img kernel7.img kernel8.img
kernel_img_DEPENDENCIES = kernel.elf
kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_img}

kernel7_img_DEPENDENCIES = kernel.elf
kernel7_img_SOURCES =
kernel7.img$(EXEEXT): ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_img}

kernel8_img_DEPENDENCIES = kernel.elf
kernel8_img_SOURCES =
kernel8.img$(EXEEXT): ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/platform/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/platform/rpi/kernel.elf ${top_builddir}/src/platform/rpi/${output_img}
