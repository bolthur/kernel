
# built sources determining things to be built first
BUILT_SOURCES = \
  ${abs_top_builddir}/thirdparty/font/libfont.la \
  ${abs_top_builddir}/thirdparty/dtc/libfdt/libfdt.la \
  ${abs_top_builddir}/src/platform/${platform_subdir}/lib${platform_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/lib${arch_subdir}.la \
  ${abs_top_builddir}/src/core/libcore.la \
  ${abs_top_builddir}/src/lib/atag/libatag.la \
  ${abs_top_builddir}/src/lib/collection/libcollection.la \
  ${abs_top_builddir}/src/lib/libc/libc.la \
  ${abs_top_builddir}/src/lib/tar/libtar.la \
  ${abs_top_builddir}/src/lib/libssp.la \
  ${abs_top_builddir}/src/lib/libubsan.la \
  libtarget.la \
  kernel.elf \
  kernel_qemu.elf

# no installable programs
noinst_PROGRAMS = kernel.elf kernel_qemu.elf ${output_img} ${output_img_qemu}
noinst_LTLIBRARIES = libtarget.la

libtarget_la_DEPENDENCIES = \
  ${abs_top_builddir}/thirdparty/font/libfont.la \
  ${abs_top_builddir}/thirdparty/dtc/libfdt/libfdt.la \
  ${abs_top_builddir}/src/platform/${platform_subdir}/lib${platform_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/lib${arch_subdir}.la \
  ${abs_top_builddir}/src/core/libcore.la \
  ${abs_top_builddir}/src/lib/atag/libatag.la \
  ${abs_top_builddir}/src/lib/collection/libcollection.la \
  ${abs_top_builddir}/src/lib/libc/libc.la \
  ${abs_top_builddir}/src/lib/tar/libtar.la \
  ${abs_top_builddir}/src/lib/libssp.la \
  ${abs_top_builddir}/src/lib/libubsan.la
libtarget_la_SOURCES =
libtarget_la_LIBADD = \
  ${CC_SPECIFIC_LDADD}  \
  ${abs_top_builddir}/thirdparty/font/libfont.la \
  ${abs_top_builddir}/thirdparty/dtc/libfdt/libfdt.la \
  ${abs_top_builddir}/src/platform/${platform_subdir}/lib${platform_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.la \
  ${abs_top_builddir}/src/arch/${arch_subdir}/lib${arch_subdir}.la \
  ${abs_top_builddir}/src/core/libcore.la \
  ${abs_top_builddir}/src/lib/atag/libatag.la \
  ${abs_top_builddir}/src/lib/collection/libcollection.la \
  ${abs_top_builddir}/src/lib/libc/libc.la \
  ${abs_top_builddir}/src/lib/tar/libtar.la \
  ${abs_top_builddir}/src/lib/libssp.la \
  ${abs_top_builddir}/src/lib/libubsan.la
libtarget_la_LDFLAGS = -all-static

# Normal kernel building for physical device
kernel_elf_SOURCES =
kernel_elf_DEPENDENCIES = libtarget.la
kernel_elf_LDFLAGS = \
  -Wl,-L,${top_srcdir}/src/target \
  -Wl,-T,${top_srcdir}/src/target/rpi/physical.${subarch_subdir}.${executable_format}.ld \
  -Wl,-Map,${output_map}
kernel_elf_LDADD = libtarget.la

# Normal kernel building for physical device
kernel_qemu_elf_SOURCES =
kernel_qemu_elf_DEPENDENCIES = libtarget.la
kernel_qemu_elf_LDFLAGS = \
  -Wl,-L,${top_srcdir}/src/target \
  -Wl,-T,${top_srcdir}/src/target/rpi/qemu.${subarch_subdir}.${executable_format}.ld \
  -Wl,-Map,${output_map_qemu}
kernel_qemu_elf_LDADD = libtarget.la

EXTRA_PROGRAMS = kernel.img kernel7.img kernel8.img kernel_qemu.img kernel7_qemu.img kernel8_qemu.img
kernel_img_DEPENDENCIES = kernel.elf
kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_img}

kernel_qemu_img_DEPENDENCIES = kernel_qemu.elf
kernel_qemu_img_SOURCES =
kernel_qemu.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_sym_qemu}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_img_qemu}

kernel7_img_DEPENDENCIES = kernel.elf
kernel7_img_SOURCES =
kernel7.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_img}

kernel7_qemu_img_DEPENDENCIES = kernel_qemu.elf
kernel7_qemu_img_SOURCES =
kernel7_qemu.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_sym_qemu}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_img_qemu}

kernel8_img_DEPENDENCIES = kernel.elf
kernel8_img_SOURCES =
kernel8.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_sym}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel.elf ${top_builddir}/src/target/rpi/${output_img}

kernel8_qemu_img_DEPENDENCIES = kernel_qemu.elf
kernel8_qemu_img_SOURCES =
kernel8_qemu.img$(EXEEXT): ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) --only-keep-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_sym_qemu}
	$(BOLTHUR_OBJCOPY) --strip-debug ${top_builddir}/src/target/rpi/kernel_qemu.elf
	$(BOLTHUR_OBJCOPY) -O binary ${top_builddir}/src/target/rpi/kernel_qemu.elf ${top_builddir}/src/target/rpi/${output_img_qemu}
