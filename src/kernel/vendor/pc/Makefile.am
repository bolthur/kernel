
AUTOMAKE_OPTIONS = nostdinc

AM_CFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc
AM_CCASFLAGS = -imacros $(top_builddir)/config.h -I$(top_srcdir)/src/kernel -I$(top_srcdir)/src/libc

noinst_LIBRARIES = libpc.a
libpc_a_SOURCES = \
  multiboot2.h \
  platform.c \
  start.S

noinst_PROGRAMS = kernel.zwerg

kernel_zwerg_SOURCES =
kernel_zwerg_DEPENDENCIES = \
  ${top_builddir}/src/kernel/vendor/rpi/kernel.${subarch_subdir}.ld \
  ${top_builddir}/src/kernel/vendor/rpi/libpc.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/${subarch_subdir}/lib${subarch_subdir}.a \
  ${top_builddir}/src/kernel/arch/${arch_subdir}/lib${arch_subdir}.a \
  ${top_builddir}/src/kernel/libkernel.a \
  ${top_builddir}/src/libc/libc.a

kernel_zwerg_LDFLAGS = \
  -T $(top_srcdir)/src/vendor/pc/kernel.${subarch_subdir}.ld \
  -Wl,-Map,kernel.map \
  -Wl,-u,Entry
