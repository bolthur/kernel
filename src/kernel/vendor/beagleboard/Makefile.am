
SUBDIRS = .

AUTOMAKE_OPTIONS = nostdinc subdir-objects

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/lib/stdc -I$(top_srcdir)/src/lib/avl -I$(top_srcdir)/src/lib/fdt -I$(top_srcdir)/src/lib/tar -I$(top_srcdir)/src/lib/endian

noinst_LIBRARIES = libbeagleboard.a
libbeagleboard_a_SOURCES =

noinst_PROGRAMS = kernel.zwerg kernel.img
kernel_zwerg_SOURCES =

kernel_zwerg_DEPENDENCIES = \
  kernel.${subarch_subdir}.ld \
  libbeagleboard.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${top_builddir}/src/kernel/libkernel.a \
  ${top_builddir}/src/lib/avl/libavl.a \
  ${top_builddir}/src/lib/fdt/libfdt.a \
  ${top_builddir}/src/lib/stdc/libstdc.a \
  ${top_builddir}/src/lib/tar/libtar.a

kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/kernel/vendor/rpi/kernel.${subarch_subdir}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry

kernel_img_SOURCES =
kernel.img$(EXEEXT): ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg
  $(MIST_OBJCOPY) ${top_builddir}/src/kernel/vendor/rpi/kernel.zwerg -O binary ${top_builddir}/src/kernel/vendor/rpi/kernel.img
